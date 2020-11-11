#include <algorithm>
#include <cassert>
#include <functional>
#include <iterator>
#include <sstream>
#include <vector>

#include "oneapi/ccl/native_device_api/l0/base_impl.hpp"
#include "oneapi/ccl/native_device_api/l0/device.hpp"
#include "oneapi/ccl/native_device_api/l0/subdevice.hpp"
#include "oneapi/ccl/native_device_api/l0/primitives_impl.hpp"
#include "oneapi/ccl/native_device_api/l0/driver.hpp"
#include "oneapi/ccl/native_device_api/l0/platform.hpp"

//#include "native_device_api/compiler_ccl_wrappers_dispatcher.hpp"

namespace native {
ccl_device_driver::driver_index_type get_driver_properties(ccl_device_driver::handle_t handle) {
    ze_driver_properties_t driver_properties{};
    ze_result_t ret = zeDriverGetProperties(handle, &driver_properties);
    if (ret != ZE_RESULT_SUCCESS) {
        throw std::runtime_error(std::string("zeDriverGetProperties, error: ") +
                                 native::to_string(ret));
    }
    //TODO only 0 index in implemented in L0
    return 0;
}

ccl_device_driver::context_storage_type ccl_device_driver::get_driver_contexts() {
    return get_ctx().lock();
}

ccl_device_driver::context_storage_type ccl_device_driver::get_driver_contexts() const {
    return get_ctx().lock();
}


ccl_device_driver::indexed_driver_handles ccl_device_driver::get_handles(
    const ccl::device_indices_type& requested_driver_indexes /* = indices()*/) {
    uint32_t driver_count = 0;
    ze_result_t err = zeDriverGet(&driver_count, nullptr);
    if (err != ZE_RESULT_SUCCESS) {
        throw std::runtime_error(std::string("zeDriverGet failed for count request, error: ") +
                                 native::to_string(err));
    }

    std::vector<ccl_device_driver::handle_t> handles;
    handles.resize(driver_count);

    err = zeDriverGet(&driver_count, handles.data());
    if (err != ZE_RESULT_SUCCESS) {
        throw std::runtime_error(std::string("zeDriverGet failed fro drivers request, error: ") +
                                 native::to_string(err));
    }

    //collect drivers by indices
    indexed_driver_handles ret;
    try {
        ret = detail::collect_indexed_data<ccl::device_index_enum::driver_index_id>(
            requested_driver_indexes,
            handles,
            std::bind(get_driver_properties, std::placeholders::_1));
    }
    catch (const std::exception& ex) {
        throw std::runtime_error(std::string("Cannot add driver: ") + ex.what());
    }
    return ret;
}

CCL_API std::shared_ptr<ccl_device_driver> ccl_device_driver::create(
    handle_t h,
    driver_index_type id,
    owner_ptr_t&& platform,
    const ccl::device_mask_t& rank_device_affinity) {
    auto ctx = platform.lock()->get_platform_contexts();
    std::shared_ptr<ccl_device_driver> driver =
        std::make_shared<ccl_device_driver>(h, id, std::move(platform), std::move(ctx));
    if (!driver->create_context()) {
        throw std::runtime_error(std::string(__PRETTY_FUNCTION__) + " - create context is invalid");
    }

    auto collected_devices_list =
        ccl_device::get_handles(*driver, get_device_indices(rank_device_affinity));
    for (const auto& val : collected_devices_list) {
        driver->devices.emplace(val.first, ccl_device::create(val.second, driver->get_ptr()));
    }

    return driver;
}

CCL_API std::shared_ptr<ccl_device_driver> ccl_device_driver::create(
    handle_t h,
    driver_index_type id,
    owner_ptr_t&& platform,
    const ccl::device_indices_type& rank_device_affinity /* = ccl::device_indices_type()*/) {
    auto ctx = platform.lock()->get_platform_contexts();
    std::shared_ptr<ccl_device_driver> driver =
        std::make_shared<ccl_device_driver>(h, id, std::move(platform), ctx);
    if (!driver->create_context()) {
        throw std::runtime_error(std::string(__PRETTY_FUNCTION__) + " - create context is invalid");
    }

    auto collected_devices_list = ccl_device::get_handles(*driver, rank_device_affinity);
    try {
        for (const auto& val : collected_devices_list) {
            if (rank_device_affinity.empty()) {
                driver->devices.emplace(val.first,
                                        ccl_device::create(val.second, driver->get_ptr()));
            }
            else {
                //collect device_index only for drvier specific index
                ccl::device_indices_type per_driver_index;
                for (const auto& affitinity : rank_device_affinity) {
                    if (std::get<ccl::device_index_enum::device_index_id>(affitinity) ==
                        val.first) {
                        per_driver_index.insert(affitinity);
                    }
                }
                driver->devices.emplace(
                    val.first, ccl_device::create(val.second, driver->get_ptr(), per_driver_index));
            }
        }
    }
    catch (const std::exception& ex) {
        std::stringstream ss;
        ss << "Cannot create devices by indices: ";
        for (const auto& index : rank_device_affinity) {
            ss << index << ", ";
        }
        ss << "\nError: " << ex.what();
        throw;
    }

    return driver;
}

CCL_API ccl_device_driver::ccl_device_driver(ccl_device_driver::handle_t h,
                                             driver_index_type id,
                                             owner_ptr_t&& platform,
                                             std::weak_ptr<ccl_context_holder>&& ctx)
        : base(h, std::move(platform), std::move(ctx)),
          driver_id(id) {
            ipc_prop.stype = ZE_STRUCTURE_TYPE_DRIVER_IPC_PROPERTIES;
            ze_result_t ret = zeDriverGetIpcProperties(handle, &ipc_prop);
            if (ret != ZE_RESULT_SUCCESS) {
                throw std::runtime_error(std::string("zeDriverGetIpcProperties, error: ") +
                                        native::to_string(ret));
            }
        }

CCL_API
ze_driver_properties_t ccl_device_driver::get_properties() const {
    ze_driver_properties_t driver_properties{};
    ze_result_t ret = zeDriverGetProperties(handle, &driver_properties);
    if (ret != ZE_RESULT_SUCCESS) {
        throw std::runtime_error(std::string("zeDriverGetProperties, error: ") +
                                 native::to_string(ret));
    }
    return driver_properties;
}

CCL_API
const ccl_device_driver::devices_storage_type& ccl_device_driver::get_devices() const noexcept {
    return devices;
}

CCL_API ccl_device_driver::device_ptr ccl_device_driver::get_device(
    const ccl::device_index_type& path) {
    return std::const_pointer_cast<ccl_device>(
        static_cast<const ccl_device_driver*>(this)->get_device(path));
}

CCL_API ccl_device_driver::const_device_ptr ccl_device_driver::get_device(
    const ccl::device_index_type& path) const {
    ccl::index_type driver_idx = std::get<ccl::device_index_enum::driver_index_id>(path);
    if (driver_idx != get_driver_id()) {
        assert(false && "incorrect driver requested");
        throw std::runtime_error(
            std::string(__PRETTY_FUNCTION__) + " - incorrect driver requested, expected: " +
            std::to_string(get_driver_id()) + ", requested: " + ccl::to_string(path));
    }

    ccl::index_type device_index = std::get<ccl::device_index_enum::device_index_id>(path);
    auto device_it = devices.find(device_index);
    if (device_it == devices.end()) {
        assert(false && "incorrect device index requested");
        throw std::runtime_error(std::string(__PRETTY_FUNCTION__) +
                                 " - incorrect device index requested: " + ccl::to_string(path) +
                                 ". Total devices count: " + std::to_string(devices.size()));
    }

    const_device_ptr found_device_ptr = device_it->second;
    ccl::index_type subdevice_index = std::get<ccl::device_index_enum::subdevice_index_id>(path);
    if (ccl::unused_index_value == subdevice_index) {
        return found_device_ptr;
    }

    return found_device_ptr->get_subdevice(path);
}

CCL_API ccl::device_mask_t ccl_device_driver::create_device_mask(
    const std::string& str_mask,
    std::ios_base::fmtflags flag /* = std::ios_base::hex*/) {
    std::stringstream ss;
    ss << str_mask;

    size_t hex_digit = 0;
    ss.setf(flag, std::ios_base::basefield);
    ss >> hex_digit;

    return ccl::device_mask_t(hex_digit);
}

CCL_API ccl_device_driver::driver_index_type ccl_device_driver::get_driver_id() const noexcept {
    return driver_id;
}

CCL_API ccl::device_indices_type ccl_device_driver::get_device_indices(
    const ccl::device_mask_t& mask) {
    ccl::device_indices_type ret;
    std::cerr << __PRETTY_FUNCTION__ << " NOT IMPLEMENTED" << std::endl;
    abort();
    /*
    for(size_t i = 0; i < mask.size(); i++)
    {
        if(mask.test(i))
        {
            ret.insert(i);
        }
    }
    */
    return ret;
}

CCL_API ccl::device_mask_t ccl_device_driver::get_device_mask(
    const ccl::device_indices_type& device_idx) {
    ccl::device_mask_t ret;
    std::cerr << __PRETTY_FUNCTION__ << " NOT IMPLEMENTED" << std::endl;
    abort();
    /*
    for(auto idx : device_idx)
    {
        ret.set(idx);
    }
    */
    return ret;
}

CCL_API std::shared_ptr<ccl_context> ccl_device_driver::create_context() {
    ze_result_t status = ZE_RESULT_SUCCESS;
    ze_context_handle_t context;
    ze_context_desc_t context_desc = { ZE_STRUCTURE_TYPE_CONTEXT_DESC, nullptr, 0 };

    status = zeContextCreate(handle, &context_desc, &context);
    if (status != ZE_RESULT_SUCCESS) {
        throw std::runtime_error(std::string("zeContextCreate, error: ") +
                                 native::to_string(status));
    }

    return create_context_from_handle(context);
}

CCL_API std::shared_ptr<ccl_context> ccl_device_driver::create_context_from_handle(
    ccl_context::handle_t h) {
    auto platform = get_owner();
    auto ctx_holder = get_ctx().lock();

    return ctx_holder->emplace(this, std::make_shared<ccl_context>(h, std::move(platform)));
}

void CCL_API ccl_device_driver::on_delete(ze_device_handle_t& sub_device_handle,
                                          ze_context_handle_t& context) {
    // status = zeContextDestroy(context);
    // assert(status == ZE_RESULT_SUCCESS);
}

std::string CCL_API ccl_device_driver::to_string(const std::string& prefix) const {
    std::stringstream out;
    out << prefix << "Driver:\n" << prefix << "{\n";
    std::string device_prefix = prefix + "\t";
    out << device_prefix << "devices count: " << devices.size() << std::endl;
    for (const auto& device_pair : devices) {
        out << device_pair.second->to_string(device_prefix);
    }
    out << "\n" << prefix << "},\n";
    return out.str();
}

std::weak_ptr<ccl_device_driver> ccl_device_driver::deserialize(const uint8_t** data,
                                                                size_t& size,
                                                                std::shared_ptr<ccl_device_platform>& out_platform) {

    //restore platform
    auto platform = ccl_device_platform::deserialize(data, size, out_platform).lock();
    if (!platform) {
        throw std::runtime_error("cannot deserialize ccl_device_driver, because owner is nullptr");
    }

    constexpr size_t expected_bytes = sizeof(driver_index_type);
    if (!*data or size < expected_bytes) {
        throw std::runtime_error("cannot deserialize ccl_device_driver, not enough data");
    }

    size_t recovered_index = *(reinterpret_cast<const driver_index_type*>(*data));
    size -= expected_bytes;
    *data += expected_bytes;

    //TODO only one instance of driver is supported
    assert(recovered_index == 0 && "Only one instance of driver is supported!");

    //find device with requested handle
    auto driver_ptr = platform->get_driver(recovered_index);
    if (!driver_ptr) {
        throw std::runtime_error(std::string(__FUNCTION__) +
                                 " - invalid driver index: " + std::to_string(recovered_index));
    }
    return driver_ptr;
}

size_t ccl_device_driver::serialize(std::vector<uint8_t>& out,
                                    size_t from_pos,
                                    size_t expected_size) const {
    // check parent existence
    const auto platform = get_owner().lock();
    if (!platform) {
        throw std::runtime_error("cannot serialize ccl_driver without owner");
    }

    constexpr size_t expected_driver_bytes = sizeof(driver_index_type);  // driver index

    size_t serialized_bytes = platform->serialize(
        out, from_pos, expected_driver_bytes + expected_size); //resize vector inside

    // serialize from position
    uint8_t* data_start = out.data() + from_pos + serialized_bytes;

    //TODO only one driver instance supported
    assert(platform->get_drivers().size() == 1 &&
           "Platform supports only one instance of the driver by 0 index");

    //serialize driver index
    *(reinterpret_cast<driver_index_type*>(data_start)) = get_driver_id();
    serialized_bytes += expected_driver_bytes;

    return serialized_bytes;

}

std::ostream& operator<<(std::ostream& out, const ccl_device_driver& node) {
    return out << node.to_string();
}

} // namespace native
