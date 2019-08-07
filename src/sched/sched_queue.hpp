#pragma once

#include "common/utils/spinlock.hpp"
#include "sched/sched.hpp"

#include <deque>
#include <unordered_map>

#define CCL_SCHED_QUEUE_INITIAL_BIN_COUNT (1024)

using sched_container_t = std::vector<ccl_sched*>;
using sched_bin_list_t = std::unordered_map<size_t, ccl_sched_bin>;
using sched_queue_lock_t = ccl_spinlock;

/* ATL comm is limited resource, each priority bucket consumes single ATL comm and uses it for all bins in bucket */
#define CCL_PRIORITY_BUCKET_COUNT (4)

/* the size of priority bucket, each bin in bucket use the same ATL comm although bins have different priorities */
#define CCL_PRIORITY_BUCKET_SIZE (8)

#define CCL_BUCKET_INITIAL_ELEMS_COUNT (1024)
class ccl_sched_list
{
public:
    friend class ccl_sched_bin;
    friend class ccl_sched_queue;
    ccl_sched_list()
    {
        elems.reserve(CCL_BUCKET_INITIAL_ELEMS_COUNT);
    }
    ccl_sched_list(ccl_sched* sched)
    {
        elems.reserve(CCL_BUCKET_INITIAL_ELEMS_COUNT);
        elems.push_back(sched);
    }
    ~ccl_sched_list()
    {
        CCL_ASSERT(elems.empty(),
                   "unexpected elem_count %zu, expected 0",
                   elems.size());
    }

    ccl_sched_list& operator= (const ccl_sched_list& other) = delete;

    ccl_sched_list(ccl_sched_list &&src) = default;
    ccl_sched_list& operator= (ccl_sched_list&& other) = default;

    void add(ccl_sched* sched)
    {
        {
            std::lock_guard<sched_queue_lock_t> lock(elemGuard);
            elems.emplace_back(sched);
        }
    }
    size_t size()
    {
        {
            std::lock_guard<sched_queue_lock_t> lock(elemGuard);
            return elems.size();
        }
    }
    bool empty()
    {
        {
            std::lock_guard<sched_queue_lock_t> lock(elemGuard);
            return elems.empty();
        }
    }
    ccl_sched* get(size_t idx)
    {
        {
            std::lock_guard<sched_queue_lock_t> lock(elemGuard);
            CCL_ASSERT(idx < elems.size());
            return elems[idx];
        }
    }

    ccl_sched* remove(size_t idx, size_t &nextIdx)
    {
        ccl_sched* ret = nullptr;
        {
            std::lock_guard<sched_queue_lock_t> lock(elemGuard);
            size_t size = elems.size();
            CCL_ASSERT(idx < size);
            ret = elems[idx];
            std::swap(elems[size - 1], elems[idx]);
            elems.resize(size - 1);
            CCL_ASSERT(elems.size() == (size - 1));
            nextIdx = idx;
        }
        return ret;
    }
    size_t erase(size_t idx)
    {
        {
            std::lock_guard<sched_queue_lock_t> lock(elemGuard);
            size_t size = elems.size();
            CCL_ASSERT(idx < size);
            std::swap(elems[size - 1], elems[idx]);
            elems.resize(size - 1);
            CCL_ASSERT(elems.size() == (size - 1));
        }
        return idx;
    }

private:
    sched_queue_lock_t elemGuard{};
    sched_container_t elems;
    char padding_queue[CACHELINE_SIZE];
};

class ccl_sched_bin
{
public:
    friend class ccl_sched_queue;
    ccl_sched_bin(ccl_sched_queue* queue, atl_comm_t* comm_ctx, size_t priority, ccl_sched* sched)
        : queue(queue),
          comm_ctx(comm_ctx),
          sched_list(sched),
          priority(priority)
    {
        CCL_ASSERT(queue);
        CCL_ASSERT(comm_ctx);
        sched->bin = this;
        sched->queue = queue;
    }

    ~ccl_sched_bin() = default;
    ccl_sched_bin() = delete;
    ccl_sched_bin& operator= (const ccl_sched_bin& other) = delete;

    ccl_sched_bin(ccl_sched_bin &&src) = default;
    ccl_sched_bin& operator= (ccl_sched_bin&& other) = default;

    size_t size() { return sched_list.size(); }
    size_t get_priority() { return priority; }
    atl_comm_t* get_comm_ctx() { return comm_ctx; }
    ccl_sched_queue* get_queue() { return queue; }

    void add(ccl_sched* sched);
    size_t erase(size_t idx);
    size_t erase(size_t idx, size_t &next);
    ccl_sched* get(size_t idx) { return sched_list.get(idx); }

private:
    ccl_sched_queue* queue = nullptr; //!< pointer to the queue which owns the bin
    atl_comm_t* comm_ctx = nullptr;    //!< ATL communication context
    ccl_sched_list sched_list;      //!< list of schedules
    size_t priority{};                 //!< the single priority for all elems
};

class ccl_sched_queue
{
public:
    ccl_sched_queue(std::vector<atl_comm_t*> comm_ctxs);
    ~ccl_sched_queue();

    ccl_sched_queue() = delete;
    ccl_sched_queue(const ccl_sched_queue& other) = delete;
    ccl_sched_queue& operator= (const ccl_sched_queue& other) = delete;

    void add(ccl_sched* sched);
    size_t erase(ccl_sched_bin* bin, size_t idx);

    /**
     * Retrieve a pointer to the bin with the highest priority and number of its elements
     * @param bin_size[out] the current number of elements in bin. May have a zero if the queue has no bins with elements
     * @return a pointer to the bin with the highest priority or nullptr if there is no bins with content
     */
    ccl_sched_bin* peek();

private:

    void add_internal(ccl_sched* sched, bool needToLock = true);

    sched_queue_lock_t postponed_queue_guard{};
    //added padding, to make sure that 'postponed_queue_guard' and 'bins_guard'
    // takes different cache lines (eliminates 'false sharing')
    char padding_queue[CACHELINE_SIZE];
    sched_queue_lock_t bins_guard{};
    char padding_table[CACHELINE_SIZE];
    std::vector<atl_comm_t*> comm_ctxs;
    sched_bin_list_t bins { CCL_SCHED_QUEUE_INITIAL_BIN_COUNT };
    size_t max_priority = 0;
    std::atomic<ccl_sched_bin*> cached_max_priority_bin{};

    /* used to get strict start ordering for transports w/o tagging support for collectives */
    std::atomic_flag postponed_queue_empty = ATOMIC_FLAG_INIT;
    std::vector<ccl_sched*> postponed_queue{};
};
