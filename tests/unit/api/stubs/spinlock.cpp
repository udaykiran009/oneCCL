#include "common/utils/spinlock.hpp"

ccl_spinlock::ccl_spinlock() {
    flag.clear();
}

void ccl_spinlock::lock() {
    while (flag.test_and_set(std::memory_order_acquire)) {
    }
}
bool ccl_spinlock::try_lock() {
    return !flag.test_and_set(std::memory_order_acquire);
}
void ccl_spinlock::unlock() {
    flag.clear(std::memory_order_release);
}
