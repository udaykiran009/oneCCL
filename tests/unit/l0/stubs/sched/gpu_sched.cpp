#include <unistd.h>

#include "sched/gpu_sched.hpp"

ccl_gpu_sched::ccl_gpu_sched(native::specific_indexed_device_storage& devices,
                             size_t group_size,
                             const ccl_coll_param& coll_param /* = ccl_coll_param()*/)
        : ccl_sched(coll_param, nullptr),
          ccl_request(),
          expected_group_size(group_size),
          group_comm_devices(devices) {
}

void ccl_gpu_sched::complete() {}

ze_fence_handle_t* entry_request = nullptr;
void ccl_gpu_sched::set_fence(ze_fence_handle_t entry_fence) //TODO temporary
{
}

bool ccl_gpu_sched::wait(size_t nanosec) {
    return true;
}
