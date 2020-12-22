#include "sched/sched_base.hpp"

std::string to_string(ccl_sched_add_mode mode) {
    switch (mode) {
        case ccl_sched_add_front: return "FRONT";
        case ccl_sched_add_back: return "BACK";
        default: return "DEFAULT";
    }
    return "DEFAULT";
}

void ccl_sched_base::set_coll_attr(const ccl_coll_attr& attr) {
    coll_attr = attr;
}

void ccl_sched_base::update_coll_param_and_attr(const ccl_coll_param& param,
                                                const ccl_coll_attr& attr) {}

size_t ccl_sched_base::get_priority() const {
    return 0;
}

ccl_buffer ccl_sched_base::alloc_buffer(size_t bytes) {
    return {};
}

ccl_buffer ccl_sched_base::alloc_sycl_buffer(size_t bytes, sycl::context& ctx) {
    return {};
}

ccl_buffer ccl_sched_base::update_buffer(ccl_buffer buffer, size_t new_size) {
    return {};
}

ccl_buffer ccl_sched_base::find_and_realloc_buffer(void* in_ptr,
                                                   size_t new_size,
                                                   size_t expected_size) {
    return ccl_buffer();
}

void ccl_sched_base::free_buffers() {}

void ccl_sched_base::add_memory_region(atl_mr_t* mr) {}

void ccl_sched_base::alloc_buffers_for_sycl_copy(const ccl_coll_param& param) {}

void ccl_sched_base::update_id() {}

void ccl_sched_base::dump(std::ostream& out, const char* name) const {}
