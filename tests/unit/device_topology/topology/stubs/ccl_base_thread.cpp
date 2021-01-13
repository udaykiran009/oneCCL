#include "exec/thread/base_thread.hpp"

ccl::status ccl_base_thread::start(int affinity) {
    return ccl::status::success;
}

ccl::status ccl_base_thread::stop() {
    return ccl::status::success;
}

ccl::status ccl_base_thread::set_affinity(int affinity) {
    return ccl::status::success;
}

int ccl_base_thread::get_affinity() {
    return 0;
}
