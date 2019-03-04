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
        MLSL_LOG(DEBUG, "deleting req %p with counter %d", this, counter);
        if (counter != 0)
        {
            MLSL_LOG(ERROR, "deleting request with unexpected completion_counter %d", counter);
        }
    }

    void complete()
    {
        int prev_counter = completion_counter.fetch_sub(1, std::memory_order_release);
        MLSL_THROW_IF_NOT(prev_counter > 0, "unexpected prev_counter %d", prev_counter);
        MLSL_LOG(DEBUG, "req %p, counter %d", this, prev_counter - 1);
    }

    bool is_completed()
    {
        auto counter = completion_counter.load(std::memory_order_acquire);
        MLSL_LOG(DEBUG, "req %p, counter %d", this, counter);
        return counter == 0;
    }

    void set_counter(int counter)
    {
        int current_counter = completion_counter.load(std::memory_order_acquire);
        MLSL_THROW_IF_NOT(current_counter == 0, "unexpected counter %d", current_counter);
        completion_counter.store(counter, std::memory_order_release);
    }

    mlsl_sched *sched = nullptr;

private:
    std::atomic_int completion_counter { 0 };
};
