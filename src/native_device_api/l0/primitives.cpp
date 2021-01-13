#include <linux/limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <algorithm>
#include <cstring>
#include <sstream>

#include "oneapi/ccl/native_device_api/l0/primitives_impl.hpp"
#include "oneapi/ccl/native_device_api/l0/device.hpp"
#include "oneapi/ccl/native_device_api/l0/context.hpp"
#include "oneapi/ccl/native_device_api/l0/platform.hpp"

#include "oneapi/ccl/native_device_api/export_api.hpp"

namespace native {
namespace detail {
CCL_BE_API void copy_memory_sync_unsafe(void* dst,
                                        const void* src,
                                        size_t size,
                                        std::weak_ptr<ccl_device> device_weak,
                                        std::shared_ptr<ccl_context> ctx) {
    //TODO: NOT THREAD-SAFE!!!
    std::shared_ptr<ccl_device> device = device_weak.lock();
    if (!device) {
        throw std::invalid_argument("device is null");
    }

    // create queue
    ze_command_queue_desc_t queue_description = device->get_default_queue_desc();
    //queue_description.mode = ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS;   TODO may be &= for flags???

    auto queue = device->create_cmd_queue(ctx, queue_description);

    //create list
    ze_command_list_desc_t list_description = device->get_default_list_desc();
    //list_description.flags = ZE_COMMAND_LIST_FLAG_MAXIMIZE_THROUGHPUT;  TODO may be &= for flags???
    auto cmd_list = device->create_cmd_list(ctx, list_description);

    ze_result_t ret =
        zeCommandListAppendMemoryCopy(cmd_list.get(), dst, src, size, nullptr, 0, nullptr);
    if (ret != ZE_RESULT_SUCCESS) {
        throw std::runtime_error(std::string("Cannot append memory copy to cmd list, error: ") +
                                 std::to_string(ret));
    }
    ret = zeCommandListClose(cmd_list.get());
    if (ret != ZE_RESULT_SUCCESS) {
        throw std::runtime_error(std::string("Cannot close cmd list, error: ") +
                                 std::to_string(ret));
    }

    //TODO: need to reinmplement
    {
        static std::mutex memory_mutex;

        std::lock_guard<std::mutex> lock(memory_mutex);
        // Execute command list in command queue
        ret = zeCommandQueueExecuteCommandLists(queue.get(), 1, cmd_list.get_ptr(), nullptr);
        if (ret != ZE_RESULT_SUCCESS) {
            throw std::runtime_error(std::string("Cannot execute command list, error: ") +
                                     std::to_string(ret));
        }

        // synchronize host and device
        ret = zeCommandQueueSynchronize(queue.get(), std::numeric_limits<uint32_t>::max());
        if (ret != ZE_RESULT_SUCCESS) {
            throw std::runtime_error(std::string("Cannot sync queue, error: ") +
                                     std::to_string(ret));
        }
    }
    // Reset (recycle) command list for new commands
    ret = zeCommandListReset(cmd_list.get());
    if (ret != ZE_RESULT_SUCCESS) {
        throw std::runtime_error(std::string("Cannot reset queue, error: ") + std::to_string(ret));
    }
}

CCL_BE_API void copy_memory_sync_unsafe(void* dst,
                                        const void* src,
                                        size_t size,
                                        std::weak_ptr<ccl_context> ctx_weak,
                                        std::shared_ptr<ccl_context> ctx) {
    // host copy
    memcpy(dst, src, size);
}

} // namespace detail

std::string get_build_log_string(const ze_module_build_log_handle_t& build_log) {
    ze_result_t result = ZE_RESULT_SUCCESS;

    size_t build_log_size = 0;
    result = zeModuleBuildLogGetString(build_log, &build_log_size, nullptr);
    if (result != ZE_RESULT_SUCCESS) {
        throw std::runtime_error("xeModuleBuildLogGetString failed: " + to_string(result));
    }

    std::vector<char> build_log_c_string(build_log_size);
    result = zeModuleBuildLogGetString(build_log, &build_log_size, build_log_c_string.data());
    if (result != ZE_RESULT_SUCCESS) {
        throw std::runtime_error("xeModuleBuildLogGetString failed: " + to_string(result));
    }
    return std::string(build_log_c_string.begin(), build_log_c_string.end());
}

bool command_queue_desc_comparator::operator()(const ze_command_queue_desc_t& lhs,
                                               const ze_command_queue_desc_t& rhs) const {
    return memcmp(&lhs, &rhs, sizeof(rhs)) < 0;
}

bool command_list_desc_comparator::operator()(const ze_command_list_desc_t& lhs,
                                              const ze_command_list_desc_t& rhs) const {
    return memcmp(&lhs, &rhs, sizeof(rhs)) < 0;
}

} // namespace native
