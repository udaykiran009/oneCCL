#pragma once

#include <atomic>

class iccl_spinlock
{
public:
    iccl_spinlock()
    {
        flag.clear();
    }

    iccl_spinlock(const iccl_spinlock&) = delete;
    ~iccl_spinlock() = default;

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
