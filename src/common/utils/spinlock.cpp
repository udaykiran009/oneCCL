#include "common/global/global.hpp"
#include "common/utils/spinlock.hpp"
#include "common/utils/yield.hpp"

ccl_spinlock::ccl_spinlock() {
    flag.clear();
}

void ccl_spinlock::lock() {
    size_t spin_count = ccl::global_data::env().spin_count;
    while (flag.test_and_set(std::memory_order_acquire)) {
        spin_count--;
        if (!spin_count) {
            ccl_yield(ccl::global_data::env().yield_type);
            spin_count = 1;
        }
    }
}
bool ccl_spinlock::try_lock() {
    return !flag.test_and_set(std::memory_order_acquire);
}
void ccl_spinlock::unlock() {
    flag.clear(std::memory_order_release);
}
