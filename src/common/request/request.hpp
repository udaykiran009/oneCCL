#pragma once

#include "sched/sched.hpp"

#include <atomic>

class alignas(CACHELINE_SIZE) mlsl_request
{
public:
    mlsl_request() = default;

    mlsl_request(const mlsl_request& other) = default;
    mlsl_request(mlsl_request&& other) = default;

    mlsl_request& operator=(const mlsl_request& other) = default;
    mlsl_request& operator=(mlsl_request&& other) = default;

    ~mlsl_request()
    {
        auto counter = completion_counter.load(std::memory_order_acquire);
        LOG_DEBUG("deleting req ", this, " with counter ", counter);
        if (counter != 0)
        {
            LOG_ERROR("deleting request with unexpected completion_counter ", counter);
        }
    }

    void complete()
    {
        int prev_counter = completion_counter.fetch_sub(1, std::memory_order_release);
        MLSL_THROW_IF_NOT(prev_counter > 0, "unexpected prev_counter ", prev_counter);
        LOG_DEBUG("req ", this, ", counter ", prev_counter - 1);
    }

    bool is_completed()
    {
        auto counter = completion_counter.load(std::memory_order_acquire);

#ifdef ENABLE_DEBUG
        if(counter != 0)
        {
            ++complete_checks_count;
            if(complete_checks_count >= CHECK_COUNT_BEFORE_DUMP)
            {
                complete_checks_count = 0;
                sched->dump_all();
            }
        }
#endif
        LOG_TRACE("req ", this, ", counter ", counter);

        return counter == 0;
    }

    void set_counter(int counter)
    {
        int current_counter = completion_counter.load(std::memory_order_acquire);
        MLSL_THROW_IF_NOT(current_counter == 0, "unexpected counter ", current_counter);
        completion_counter.store(counter, std::memory_order_release);
    }

    mlsl_sched *sched = nullptr;

private:
    std::atomic_int completion_counter { 0 };

#ifdef ENABLE_DEBUG
    size_t complete_checks_count = 0;
    static constexpr const size_t CHECK_COUNT_BEFORE_DUMP = 10000000;
#endif
};
