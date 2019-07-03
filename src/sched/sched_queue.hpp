#pragma once

#include "common/utils/spinlock.hpp"
#include "sched/sched.hpp"

#include <deque>
#include <unordered_map>

#define ICCL_SCHED_QUEUE_INITIAL_BIN_COUNT (1024)

using sched_container_t = std::deque<iccl_sched*>;
using sched_bin_list_t = std::unordered_map<size_t, iccl_sched_bin>;
using sched_queue_lock_t = iccl_spinlock;

/* ATL comm is limited resource, each priority bucket consumes single ATL comm and uses it for all bins in bucket */
#define ICCL_PRIORITY_BUCKET_COUNT (4)

/* the size of priority bucket, each bin in bucket use the same ATL comm although bins have different priorities */
#define ICCL_PRIORITY_BUCKET_SIZE (8)

class iccl_sched_list
{
public:
    iccl_sched_list() = default;
    ~iccl_sched_list()
    {
        ICCL_ASSERT(elems.empty(),
            "unexpected elem_count %zu, expected 0",
            elems.size());
    }

    iccl_sched_list& operator= (const iccl_sched_list& other) = delete;

    void add(iccl_sched* sched) { elems.emplace_back(sched); }
    size_t size() { return elems.size(); }
    bool empty() { return elems.empty(); }
    iccl_sched* get(size_t idx) { return elems[idx]; }

    size_t erase(size_t idx)
    {
        size_t size = elems.size();
        ICCL_ASSERT(idx < size);
        std::swap(elems[size - 1], elems[idx]);
        elems.resize(size - 1);
        ICCL_ASSERT(elems.size() == (size - 1));
        return idx;
    }

private:
    sched_container_t elems{};
};

class iccl_sched_bin
{
public:
    iccl_sched_bin(iccl_sched_queue* queue, atl_comm_t* comm_ctx, size_t priority)
        : queue(queue),
          comm_ctx(comm_ctx),
          priority(priority)
    {
        ICCL_ASSERT(queue);
        ICCL_ASSERT(comm_ctx);
    }

    ~iccl_sched_bin() = default;
    iccl_sched_bin() = delete;
    iccl_sched_bin& operator= (const iccl_sched_bin& other) = delete;

    size_t size() { return sched_list.size(); }
    size_t get_priority() { return priority; }
    atl_comm_t* get_comm_ctx() { return comm_ctx; }
    iccl_sched_queue* get_queue() { return queue; }

    void add(iccl_sched* sched);
    size_t erase(size_t idx);
    iccl_sched* get(size_t idx) { return sched_list.get(idx); }

private:
    iccl_sched_queue* queue = nullptr; //!< pointer to the queue which owns the bin
    atl_comm_t* comm_ctx = nullptr;    //!< ATL communication context
    iccl_sched_list sched_list{};      //!< list of schedules
    size_t priority{};                 //!< the single priority for all elems
};

class iccl_sched_queue
{
public:
    iccl_sched_queue(std::vector<atl_comm_t*> comm_ctxs);
    ~iccl_sched_queue();

    iccl_sched_queue() = delete;
    iccl_sched_queue(const iccl_sched_queue& other) = delete;
    iccl_sched_queue& operator= (const iccl_sched_queue& other) = delete;

    void add(iccl_sched* sched);
    size_t erase(iccl_sched_bin* bin, size_t idx);

    /**
     * Retrieve a pointer to the bin with the highest priority and number of its elements
     * @param bin_size[out] the current number of elements in bin. May have a zero if the queue has no bins with elements
     * @return a pointer to the bin with the highest priority or nullptr if there is no bins with content
     */
    iccl_sched_bin* peek(size_t& bin_size);

private:

    void add_internal(iccl_sched* sched);

    sched_queue_lock_t guard{};
    std::vector<atl_comm_t*> comm_ctxs;
    sched_bin_list_t bins { ICCL_SCHED_QUEUE_INITIAL_BIN_COUNT };
    size_t max_priority = 0;
    iccl_sched_bin* cached_max_priority_bin = nullptr;

    /* used to get strict start ordering for transports w/o tagging support for collectives */
    std::deque<iccl_sched*> postponed_queue{};
};
