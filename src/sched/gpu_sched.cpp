#include <unistd.h>

#include "sched/gpu_sched.hpp"

ccl_gpu_sched::ccl_gpu_sched(native::specific_indexed_device_storage& devices,
                             size_t group_size,
                             const ccl_coll_param& coll_param /* = ccl_coll_param()*/)
        : ccl_sched(coll_param, nullptr),
          ccl_request(),
          expected_group_size(group_size),
          group_comm_devices(devices) {
    //preallocation
    entry_fences.reserve(expected_group_size);
    //WHY deque??? entries.reserve(expected_group_size);
}

void ccl_gpu_sched::complete() {}

ze_fence_handle_t* entry_request = nullptr;
void ccl_gpu_sched::set_fence(ze_fence_handle_t entry_fence) //TODO temporary
{
    assert(entry_fence);
    entry_fences.push_back(entry_fence);
}

bool ccl_gpu_sched::wait(size_t nanosec) {
    if (nanosec > 0) {
        throw std::runtime_error("nanosec != 0, not yet supported");
    }

    // TODO: in case we really need to support != 0 case it's probably better to
    // rework entry->do_progress to work with timeout value
    for (auto& e : entries) {
        if (e->get_status() != ccl_sched_entry_status_complete) {
            return false;
        }
    }

    return true;
}
