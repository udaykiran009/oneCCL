#pragma once
#include "kernel_utils.hpp"

namespace multi_tile_utils {

void prepare_queues_and_lists(const std::vector<native::ccl_device_driver::device_ptr>& in_devices,
                              std::shared_ptr<native::ccl_context> in_ctx,
                              std::map<size_t, native::ccl_device::device_queue>& out_queues,
                              std::map<size_t, native::ccl_device::device_cmd_list>& out_lists,
                              std::ostream& out) {
    out_queues.clear();
    out_lists.clear();
    for (const auto& device : in_devices) {
        size_t device_index = out_queues.size();
        try {
            // get queue group properties
            native::ccl_device::queue_group_properties queue_props = device->get_queue_group_prop();
            ze_command_queue_desc_t queue_descr = select_compute_group_prop(
                device_index, queue_props, device->get_default_queue_desc());

            out << "Create ASYNC command queue for device: " << device_index
                << " - ordinal: " << queue_descr.ordinal << ", index: " << queue_descr.index
                << std::endl;

            // create queue with prop
            out_queues.emplace(device_index, device->create_cmd_queue(in_ctx, queue_descr));

            // create list with prop
            ze_command_list_desc_t list_descr = device->get_default_list_desc();
            list_descr.commandQueueGroupOrdinal = queue_descr.ordinal;
            out_lists.emplace(device_index, device->create_cmd_list(in_ctx, list_descr));
        }
        catch (const std::exception& ex) {
            throw std::runtime_error(std::string("Error: ") + ex.what());
        }
    }
}
} // namespace multi_tile_utils
