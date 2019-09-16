#pragma once

#include "sched/queue/queue.hpp"

using sched_queue_t = std::vector<ccl_sched*>;

/* used to ensure strict start ordering for transports w/o tagged direct collectives - i.e. for all transports */
class ccl_strict_sched_queue
{
public:
    ccl_strict_sched_queue() {}
    ccl_strict_sched_queue(const ccl_strict_sched_queue& other) = delete;
    ccl_sched_queue& operator= (const ccl_strict_sched_queue& other) = delete;
    ~ccl_strict_sched_queue() {}

    void add(ccl_sched* sched);
    void clear();
    sched_queue_t& peek();

private:

    sched_queue_lock_t queue_guard{};

    std::atomic_bool is_queue_empty { true };

    /* used to buffer schedules which require strict start ordering */
    sched_queue_t queue{};

    /* but real strict starting will happen from this queue */
    sched_queue_t active_queue{};
};
