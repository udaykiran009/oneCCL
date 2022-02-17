#include "common/global/global.hpp"
#include "common/log/log.hpp"
#include "sched/entry/ze/ze_call.hpp"
#include "sched/entry/ze/ze_primitives.hpp"

namespace ccl {
namespace ze {

std::mutex ze_call::mutex;

ze_call::ze_call() {
    if ((global_data::env().ze_serialize_mode & ze_call::serialize_mode::lock) != 0) {
        LOG_DEBUG("ze call is locked");
        mutex.lock();
    }
}

ze_call::~ze_call() {
    if ((global_data::env().ze_serialize_mode & ze_call::serialize_mode::lock) != 0) {
        LOG_DEBUG("ze call is unlocked");
        mutex.unlock();
    }
}

ze_result_t ze_call::do_call(ze_result_t ze_result, const char* ze_name) const {
    LOG_DEBUG("call ze function: ", ze_name);
    if (ze_result != ZE_RESULT_SUCCESS) {
        CCL_THROW("ze error at ", ze_name, ", code: ", to_string(ze_result));
    }
    return ze_result;
}

// provides different level zero synchronize methods
template <>
ze_result_t zeHostSynchronize(ze_event_handle_t handle) {
    return zeHostSynchronizeImpl(zeEventHostSynchronize, handle);
}

template <>
ze_result_t zeHostSynchronize(ze_command_queue_handle_t handle) {
    return zeHostSynchronizeImpl(zeCommandQueueSynchronize, handle);
}

template <>
ze_result_t zeHostSynchronize(ze_fence_handle_t handle) {
    return zeHostSynchronizeImpl(zeFenceHostSynchronize, handle);
}

} // namespace ze
} // namespace ccl
