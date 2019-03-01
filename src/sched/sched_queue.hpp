#pragma once

#include "sched/sched.hpp"

#include <deque>
#include <unordered_map>

#define MLSL_SCHED_QUEUE_INITIAL_BUCKET_COUNT (1024)

using sched_list_t = std::deque<mlsl_sched*>;
using sched_bin_list_t = std::unordered_map<size_t, mlsl_sched_bin>;

/* ATL comm is limited resource, each priority bucket consumes single ATL comm and uses it for all bins in bucket */
#define MLSL_PRIORITY_BUCKET_COUNT (4)

/* the size of priority bucket, each bin in bucket use the same ATL comm although bins have different priorities */
#define MLSL_PRIORITY_BUCKET_SIZE (8)

class mlsl_sched_bin
{
public:
    mlsl_sched_bin(mlsl_sched_queue* queue, atl_comm_t* comm_ctx, size_t priority)
        : queue(queue),
          comm_ctx(comm_ctx),
          priority(priority)
    {
        MLSL_ASSERT(queue);
        MLSL_ASSERT(comm_ctx);
    }

    mlsl_sched_bin(const mlsl_sched_bin& other) = default;

    ~mlsl_sched_bin()
    {
        MLSL_ASSERT_FMT(scheds.empty(),
            "unexpected scheds size %zu, expected 0",
            scheds.size());
    }

    mlsl_sched_bin() = delete;
    mlsl_sched_bin& operator= (const mlsl_sched_bin& other) = delete;

    size_t size() { return scheds.size(); }
    size_t get_priority() { return priority; }
    atl_comm_t* get_comm_ctx() { return comm_ctx; }
    sched_list_t& get_scheds() { return scheds; }
    mlsl_sched_queue* get_queue() { return queue; }

    void add(mlsl_sched* sched);
    sched_list_t::iterator erase(sched_list_t::iterator sched_it);

private:
    mlsl_sched_queue* queue = nullptr; //!< pointer to the queue which owns the bin
    atl_comm_t* comm_ctx = nullptr;    //!< ATL communication context
    sched_list_t scheds{};             //!< list of schedules
    size_t priority{};                 //!< the single priority for all elems
};

class mlsl_sched_queue
{
public:
    mlsl_sched_queue(std::vector<atl_comm_t*> comm_ctxs);
    ~mlsl_sched_queue();

    mlsl_sched_queue() = delete;
    mlsl_sched_queue(const mlsl_sched_queue& other) = delete;
    mlsl_sched_queue& operator= (const mlsl_sched_queue& other) = delete;

    void add(mlsl_sched* sched, size_t priority);
    sched_list_t::iterator erase(sched_list_t::iterator sched_it);

    /**
     * Retrieve a pointer to the bin with the highest priority and number of its elements
     * @param bin_size[out] the current number of elements in bin. May have a zero if the queue has no bins with elements
     * @return a pointer to the bin with the highest priority or nullptr if there is no bins with content
     */
    mlsl_sched_bin* peek(size_t& bin_size);

private:

    /* TODO: spinlock */
    std::mutex guard{};
    std::vector<atl_comm_t*> comm_ctxs;
    sched_bin_list_t bins { MLSL_SCHED_QUEUE_INITIAL_BUCKET_COUNT };
    size_t max_priority = 0;
    mlsl_sched_bin* cached_max_priority_bin = nullptr;
};
