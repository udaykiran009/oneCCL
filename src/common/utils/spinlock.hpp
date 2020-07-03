#pragma once

#include <atomic>
#include <immintrin.h>

class ccl_spinlock {
public:
    ccl_spinlock();
    ccl_spinlock(const ccl_spinlock&) = delete;
    ~ccl_spinlock() = default;

    void lock();
    bool try_lock();
    void unlock();

private:
    std::atomic_flag flag;
};
