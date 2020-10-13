#include "common/comm/l0/devices/devices_declaration.hpp"
#include "common/comm/l0/context/device_storage.hpp"
#include "native_device_api/compiler_ccl_wrappers_dispatcher.hpp"
namespace native {

std::shared_ptr<specific_plain_device_storage> device_storage::create_devices_by_indices(
    size_t thread_id,
    const ccl::device_indices_t& indices) {
    std::shared_ptr<specific_plain_device_storage> out_devices =
        std::make_shared<specific_plain_device_storage>();
    size_t index_in_group = 0;
    for (const auto& idx : indices) {
        LOG_DEBUG("Assign device by id: ", idx, " from group size: ", indices.size());

        try {
            ccl_device_driver::device_ptr runtime_device = get_runtime_device(idx);
            if (!runtime_device) {
                throw std::runtime_error(std::string("Cannot find device by id: ") +
                                         ccl::to_string(idx));
            }

            // find index in real devices at first
            device_container<ccl_gpu_comm>& real_devices =
                ccl_tuple_get<device_container<ccl_gpu_comm>>(gpu_device_storage);
            auto real_it = real_devices.find(runtime_device->handle);
            if (real_it == real_devices.end()) {
                // first time requested device, mark it as real
                std::get<ccl_gpu_comm::type_idx()>(*out_devices)
                    .push_back(create_gpu_device<ccl_gpu_comm>(*runtime_device, index_in_group++));
            }
            else {
                // real device wrapper created already, make virtual wrapper
                auto& real = real_it->second;
                std::get<ccl_virtual_gpu_comm::type_idx()>(*out_devices)
                    .push_back(create_gpu_device<ccl_virtual_gpu_comm>(
                        real->get_device(), index_in_group++, *real));
            }
        }
        catch (const std::exception& ex) {
            LOG_ERROR("Cannot create device: ", ex.what());
            assert(false && "device_storage::create_devices_by_indices - exception");
            throw;
        }
    }

    // remember in exclusive threads ownership
    bool inserted = thread_gpu_comms.insert({ thread_id, out_devices }).second;
    if (!inserted) {
        abort(); // TODO consider use-case
    }
    return out_devices;
}

size_t device_storage::get_storage_size() const {
    return detail::get_aggregated_size<specific_device_storage, SUPPORTED_DEVICES_DECL_LIST>(
        gpu_device_storage); /*
        return get_size<ccl_gpu_comm>() +
               get_size<ccl_ipc_gpu_comm>() +
               get_size<ccl_virtual_gpu_comm>() +
               get_size<ccl_thread_comm<ccl_gpu_comm>>() +
               get_size<ccl_thread_comm<ccl_virtual_gpu_comm>>() +
               get_size<ccl_ipc_source_gpu_comm<ccl_gpu_comm>>() +
               get_size<ccl_ipc_source_gpu_comm<ccl_virtual_gpu_comm>>() +
               get_size<ccl_ipc_gpu_comm>();
*/
}
/*
    template<class DeviceType, class ...Types>
    size_t get_aggregated_size() const
    {
        return get_size<DeviceType>() + detail::get_aggregated_size_helper<Types...>(gpu_device_storage);
    }
*/
} // namespace native
