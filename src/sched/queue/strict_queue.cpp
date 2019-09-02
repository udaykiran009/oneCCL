#include "sched/queue/strict_queue.hpp"

void ccl_strict_sched_queue::add(ccl_sched* sched)
{
    CCL_ASSERT(sched->strict_start_order);

    std::lock_guard<sched_queue_lock_t> lock{queue_guard};
    queue.push_back(sched);
    is_queue_empty.clear();
}

void ccl_strict_sched_queue::clear()
{
    queue.clear();
    active_queue.clear();
    is_queue_empty.clear();
}

sched_queue_t& ccl_strict_sched_queue::peek()
{
    if (active_queue.empty())
    {
        if (!is_queue_empty.test_and_set())
        {
            {
                std::lock_guard<sched_queue_lock_t> lock{queue_guard};
                if (!queue.empty())
                {
                    active_queue.swap(queue);
                }
                else
                {
                    CCL_ASSERT("unexpected path");
                }
            }

            for (const auto& sched : active_queue)
            {
                sched->set_in_bin_status(ccl_sched_in_bin_none);
            }
        }
    }
    return active_queue;
}
