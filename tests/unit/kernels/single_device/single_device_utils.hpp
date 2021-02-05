#pragma once
#include "kernel_utils.hpp"

namespace single_device_utils {

void prepare_kernel_queues_lists(
    const std::vector<native::ccl_device_driver::device_ptr>& in_devices,
    std::shared_ptr<native::ccl_context> in_ctx,
    std::map<size_t, native::ccl_device::device_queue>& out_queues,
    std::map<size_t, native::ccl_device::device_cmd_list>& out_lists,
    std::ostream& out) {
    out_queues.clear();
    out_lists.clear();
    for (size_t thread_idx = 0; thread_idx < in_devices.size(); thread_idx++) {
        try {
            out_queues.emplace(thread_idx, in_devices[thread_idx]->create_cmd_queue(in_ctx));
            out_lists.emplace(thread_idx, in_devices[thread_idx]->create_cmd_list(in_ctx));
        }
        catch (const std::exception& ex) {
            throw std::runtime_error(std::string("Error: ") + ex.what());
        }
    }
}
} // namespace single_device_utils
