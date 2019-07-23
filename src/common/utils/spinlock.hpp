#pragma once

#include <atomic>

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
        while (flag.test_and_set(std::memory_order_acquire));
    }

    void unlock()
    {
        flag.clear(std::memory_order_release);
    }

private:
    std::atomic_flag flag;
};
