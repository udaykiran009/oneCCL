#pragma once

#include <atomic>

class mlsl_spinlock
{
public:
    mlsl_spinlock()
    {
    	flag.clear();
    }

    mlsl_spinlock(const mlsl_spinlock&) = delete;
    ~mlsl_spinlock() = default;

    void lock()
    {
        while (flag.test_and_set(std::memory_order_acquire));
    }

    void unlock()
    {
        flag.clear(std::memory_order_release);
    }

private:
    std::atomic_flag flag;
};
