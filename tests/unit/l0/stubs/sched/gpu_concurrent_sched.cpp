#include <functional>
#include "sched/gpu_concurrent_sched.hpp"

ccl_gpu_concurrent_sched::ccl_gpu_concurrent_sched(
    size_t expected_threads_count,
    const ccl_coll_param& coll_param /* = ccl_coll_param()*/)
        : ccl_sched_base(coll_param),
          ccl_request(),
          partial_scheds(expected_threads_count) {

}

ccl_gpu_concurrent_sched::~ccl_gpu_concurrent_sched() {}

std::shared_ptr<ccl_gpu_sched> ccl_gpu_concurrent_sched::get_gpu_sched(size_t thread_id) {
    return {};
}

std::shared_ptr<ccl_gpu_sched> ccl_gpu_concurrent_sched::create_gpu_sched(
    size_t thread_id,
    native::specific_indexed_device_storage& thread_devices,
    size_t expected_group_size,
    const ccl_coll_param& coll_param /* = ccl_coll_param()*/) {
    return {};
}

ccl_gpu_concurrent_sched::ccl_gpu_concurrent_sched_ptr ccl_gpu_concurrent_sched::create(
    size_t thread_count,
    const ccl_coll_param& param /* = ccl_coll_param()*/) {
    return {};
}
