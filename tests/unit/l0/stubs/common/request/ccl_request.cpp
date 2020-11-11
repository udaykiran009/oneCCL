
#include "common/request/request.hpp"

#ifdef ENABLE_DEBUG
void ccl_request::set_dump_callback(dump_func &&callback) {
}
#endif

ccl_request::~ccl_request() {
}

bool ccl_request::complete() {
    return true;
}

bool ccl_request::is_completed() const {
    return true;
}

void ccl_request::set_counter(int counter) {
}

void ccl_request::increase_counter(int increment) {
}
