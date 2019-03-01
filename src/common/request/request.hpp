#pragma once

#include "sched/sched.hpp"

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
        MLSL_LOG(DEBUG, "deleting req %p with counter %d", this, completion_counter);
        if(completion_counter > 0)
        {
            MLSL_LOG(ERROR, "deleting non completed request");
        }
    }

    void complete()
    {
        int prev_counter __attribute__ ((unused));
        prev_counter = __atomic_fetch_sub(&completion_counter, 1, __ATOMIC_RELEASE);
        MLSL_THROW_IF_NOT(prev_counter > 0, "unexpected prev_counter %d", prev_counter);
        MLSL_LOG(DEBUG, "req %p, counter %d", this, prev_counter - 1);
    }

    bool is_completed()
    {
        auto value = __atomic_load_n(&completion_counter, __ATOMIC_ACQUIRE);
        MLSL_LOG(DEBUG, "req %p, count %d", this, value);
        return value == 0;
    }

    void set_count(int count)
    {
        __atomic_store_n(&completion_counter, count, __ATOMIC_RELEASE);
    }

    mlsl_sched *sched = nullptr;

private:
    int completion_counter = 0;
};
