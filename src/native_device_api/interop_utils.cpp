#include "oneapi/ccl/native_device_api/interop_utils.hpp"
#include "common/log/log.hpp"
#include "common/utils/enums.hpp"

#if defined(MULTI_GPU_SUPPORT)
#include "oneapi/ccl/native_device_api/l0/primitives.hpp"
#endif

#if defined(MULTI_GPU_SUPPORT) && defined(CCL_ENABLE_SYCL)
#include <CL/sycl/backend/level_zero.hpp>
#include "oneapi/ccl/native_device_api/l0/primitives.hpp"
#endif

namespace native {
namespace detail {
#if defined(MULTI_GPU_SUPPORT) && defined(CCL_ENABLE_SYCL)

size_t get_sycl_device_id(const cl::sycl::device& device) {
    if (!device.is_gpu()) {
        CCL_THROW("failed for sycl device: it is not gpu");
    }
    size_t device_id = std::numeric_limits<size_t>::max();
    try {
        // extract native handle L0
        auto l0_handle = device.template get_native<cl::sycl::backend::level_zero>();

        ze_device_properties_t device_properties;
        ze_result_t ret = zeDeviceGetProperties(l0_handle, &device_properties);
        if (ret != ZE_RESULT_SUCCESS) {
            CCL_THROW("zeDeviceGetProperties failed, error: " + native::to_string(ret));
        }
        device_id = device_properties.deviceId;
    }
    catch (const cl::sycl::exception& e) {
        //TODO: errc::backend_mismatch
        CCL_THROW(std::string("cannot retrieve l0 handle: ") + e.what());
    }
    return device_id;
}

size_t get_sycl_subdevice_id(const cl::sycl::device& device) {
    if (!device.is_gpu()) {
        CCL_THROW("failed for sycl device: it is not gpu");
    }

    size_t subdevice_id = std::numeric_limits<size_t>::max();
    try {
        // extract native handle L0
        auto l0_handle = device.template get_native<cl::sycl::backend::level_zero>();

        ze_device_properties_t device_properties;
        ze_result_t ret = zeDeviceGetProperties(l0_handle, &device_properties);
        if (ret != ZE_RESULT_SUCCESS) {
            CCL_THROW("zeDeviceGetProperties failed, error: " + native::to_string(ret));
        }

        if (!(device_properties.flags & ZE_DEVICE_PROPERTY_FLAG_SUBDEVICE)) {
            return ccl::unused_index_value;
        }

        subdevice_id = device_properties.subdeviceId;
    }
    catch (const cl::sycl::exception& e) {
        //TODO: errc::backend_mismatch
        CCL_THROW(std::string("cannot retrieve l0 handle: ") + e.what());
    }
    return subdevice_id;
}
#endif

#ifdef CCL_ENABLE_SYCL
using usm_type_str = utils::enum_to_str<utils::enum_to_underlying(cl::sycl::usm::alloc::unknown)>;
std::string usm_to_string(cl::sycl::usm::alloc val) {
    return usm_type_str({
                            "HOST",
                            "DEVICE",
                            "SHARED",
                        })
        .choose(val, "UNKNOWN");
}
#endif

using usm_move_str = utils::enum_to_str<utils::enum_to_underlying(usm_support_mode::last_value)>;
std::string to_string(usm_support_mode val) {
    return usm_move_str({
                            "prohibited",
                            "direct",
                            "shared",
                            "convertation",
                        })
        .choose(val, "UNKNOWN");
}

size_t get_platform_type_index(const ccl::unified_device_type::ccl_native_t& device) {
    size_t index = 2; //`gpu` for default L0 backend

#ifdef CCL_ENABLE_SYCL
    if (device.is_host()) {
        index = 0;
    }
    else if (device.is_cpu()) {
        index = 1;
    }
    else if (device.is_gpu()) {
        index = 2;
    }
    else if (device.is_accelerator()) {
        index = 3;
    }
    else {
        CCL_THROW("invalid device type");
    }
#endif

    return index;
}

#if defined(MULTI_GPU_SUPPORT) || defined(CCL_ENABLE_SYCL)
assoc_result check_assoc_device_memory(const void* mem,
                                       const ccl::unified_device_type::ccl_native_t& device,
                                       const ccl::unified_context_type::ccl_native_t& ctx,
                                       bool has_deps) {
    assoc_result ret{ usm_support_mode::direct, mem, "" };

#ifdef CCL_ENABLE_SYCL

    sycl::usm::alloc pointer_type = sycl::get_pointer_type(mem, ctx);

    using usm_truth_table =
        std::array<usm_support_mode, utils::enum_to_underlying(sycl::usm::alloc::unknown) + 1>;

    constexpr int platform_config_count = 4; /*host, cpu, gpu, accel*/
    constexpr std::array<usm_truth_table, platform_config_count> usm_target_table{ {
        { { usm_support_mode::direct,
            usm_support_mode::prohibited,
            usm_support_mode::shared,
            usm_support_mode::direct } }, //host conf:  host, device, shared, unknown
        { { usm_support_mode::direct,
            usm_support_mode::prohibited,
            usm_support_mode::shared,
            usm_support_mode::direct } }, //cpu conf:  host, device, shared, unknown
        { { usm_support_mode::prohibited,
            usm_support_mode::need_conversion,
            usm_support_mode::shared,
            usm_support_mode::prohibited } }, //gpu conf:  host, device, shared, unknown
        { { usm_support_mode::prohibited,
            usm_support_mode::prohibited,
            usm_support_mode::shared,
            usm_support_mode::prohibited } } //accel conf:  host, device, shared, unknown
    } };

    auto platform_type_index = get_platform_type_index(device);

    auto pointer_type_idx = utils::enum_to_underlying(pointer_type);
    CCL_THROW_IF_NOT(pointer_type_idx < usm_target_table[platform_type_index].size(),
                     "usm_type index ",
                     pointer_type_idx,
                     " is larger that array size ",
                     usm_target_table[platform_type_index].size());

    std::get<assoc_result_index::SUPPORT_MODE>(ret) =
        usm_target_table[platform_type_index][pointer_type_idx];

    if (has_deps && device.is_gpu() && (pointer_type == sycl::usm::alloc::shared)) {
        std::get<assoc_result_index::SUPPORT_MODE>(ret) = usm_support_mode::need_conversion;
    }

    if (std::get<assoc_result_index::SUPPORT_MODE>(ret) == usm_support_mode::prohibited) {
        std::stringstream ss;
        ss << "Incompatible USM type requested: " << usm_to_string(pointer_type)
           << ", for ccl_device: " << std::to_string(platform_type_index);
        std::get<assoc_result_index::ERROR_CAUSE>(ret) = ss.str();
    }
#else
    //TODO calls method `assoc` for ccl_device
#endif
    return ret;
}

usm_support_mode check_assoc_device_memory(const std::vector<void*>& mems,
                                           const ccl::unified_device_type::ccl_native_t& device,
                                           const ccl::unified_context_type::ccl_native_t& ctx,
                                           bool has_deps) {
    usm_support_mode ret = usm_support_mode::direct;
    std::string err_msg;

    for (size_t idx = 0; idx < mems.size(); idx++) {
        usm_support_mode mode;
        std::tie(mode, std::ignore, err_msg) =
            check_assoc_device_memory(mems[idx], device, ctx, has_deps);

        if (idx > 0)
            CCL_THROW_IF_NOT(mode == ret, "different USM modes between buffers: ", err_msg);

        ret = mode;

        CCL_THROW_IF_NOT((mode == usm_support_mode::direct) || (mode == usm_support_mode::shared) ||
                             (mode == usm_support_mode::need_conversion),
                         "unsupported USM configuration: ",
                         err_msg);
    }

    return ret;
}

#endif //defined(MULTI_GPU_SUPPORT) || defined(CCL_ENABLE_SYCL)

std::string to_string(const assoc_result& res) {
    std::stringstream ss;
    ss << "Mem: " << std::get<assoc_result_index::POINTER_VALUE>(res)
       << ", is: " << to_string(std::get<assoc_result_index::SUPPORT_MODE>(res));
    const std::string& err_cause = std::get<assoc_result_index::ERROR_CAUSE>(res);
    if (!err_cause.empty()) {
        ss << ", error cause: " << err_cause;
    }
    return ss.str();
}
} // namespace detail
} // namespace native
