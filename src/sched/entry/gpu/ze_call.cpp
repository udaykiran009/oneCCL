#include "ze_call.hpp"
#include "common/global/global.hpp"

namespace ccl {
namespace ze {

// implementation of ze_call class
std::mutex ze_call::mutex;

ze_call::ze_call() {
    if (((global_data::env().ze_serialize_mode & ze_serialize_lock)) != 0) {
        LOG_DEBUG("level zero call is locked");
        mutex.lock();
    }
}

ze_call::~ze_call() {
    if (((global_data::env().ze_serialize_mode & ze_serialize_lock)) != 0) {
        LOG_DEBUG("level zero call is unlocked");
        mutex.unlock();
    }
}

void ze_call::do_call(ze_result_t ze_result, const char* ze_name) {
    if (ze_result != ZE_RESULT_SUCCESS) {
        CCL_THROW("level-zero error at ", ze_name, ", code: ", ccl::ze::to_string(ze_result));
    }
    LOG_DEBUG("call level zero function: ", ze_name);
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
