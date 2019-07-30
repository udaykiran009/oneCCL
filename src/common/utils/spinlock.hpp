#pragma once

#include <atomic>

#include "common/env/env.hpp"
#include "common/utils/yield.hpp"

class ccl_spinlock
{
public:
    ccl_spinlock()
    {
        flag.clear();
    }

    ccl_spinlock(const ccl_spinlock&) = delete;
    ~ccl_spinlock() = default;

    void lock()
    {
        size_t spin_count = env_data.spin_count;
        while (flag.test_and_set(std::memory_order_acquire))
        {
            spin_count--;
            if (!spin_count)
            {
                ccl_yield(env_data.yield_type);
                spin_count = 1;
            }
        }
    }

    void unlock()
    {
        flag.clear(std::memory_order_release);
    }

private:
    std::atomic_flag flag;
};
