#include "oneapi/ccl/native_device_api/interop_utils.hpp"
#include "common/utils/enums.hpp"
#if defined(MULTI_GPU_SUPPORT) && defined(CCL_ENABLE_SYCL)
#include <CL/sycl/backend/Intel_level0.hpp>
#endif

namespace native {
namespace details {
#if defined(MULTI_GPU_SUPPORT) && defined(CCL_ENABLE_SYCL)

size_t get_sycl_device_id(const cl::sycl::device& device) {
    if (!device.is_gpu()) {
        throw std::runtime_error(std::string(__FUNCTION__) +
                                 " - failed for sycl device: it is not gpu!");
    }

    size_t device_id = std::numeric_limits<size_t>::max();

    // extract native handle L0
    auto l0_handle_ptr = device.template get_native<cl::sycl::backend::level0>();
    if (!l0_handle_ptr) {
        throw std::runtime_error(std::string(__FUNCTION__) +
                                 " - failed for sycl device: handle is nullptr!");
    }

    ze_device_properties_t device_properties;
    device_properties.version = ZE_DEVICE_PROPERTIES_VERSION_CURRENT;
    ze_result_t ret = zeDeviceGetProperties(l0_handle_ptr, &device_properties);
    if (ret != ZE_RESULT_SUCCESS) {
        throw std::runtime_error(
            std::string(__FUNCTION__) +
            " - zeDeviceGetProperties failed, error: " + native::to_string(ret));
    }

    //use deviceId to return native device
    device_id = device_properties.deviceId;

    //TODO only device not subdevices
    return device_id;
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
std::string to_string(usm_support_mode val)
{
    return usm_move_str({
                            "prohibited",
                            "direct",
                            "shared",
                            "convertation",
                        })
        .choose(val, "UNKNOWN");
}

int get_platform_type_index(const ccl::unified_device_type::ccl_native_t& device) {
    int index = 2; //`gpu` for default L0 backend
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
        throw ccl::ccl_error(std::string(__PRETTY_FUNCTION__) + " - Invalid device type");
    }
#endif
    return index;
}

assoc_retult check_assoc_device_memory(
    const void* mem,
    const ccl::unified_device_type::ccl_native_t& device,
    const ccl::unified_device_context_type::ccl_native_t& ctx) {
    assoc_retult ret{ usm_support_mode::direct, mem, "" };
#ifdef CCL_ENABLE_SYCL
    cl::sycl::usm::alloc pointer_type = cl::sycl::get_pointer_type(mem, ctx);

    using usm_thruth_table =
        std::array<usm_support_mode, utils::enum_to_underlying(cl::sycl::usm::alloc::unknown)>;
    constexpr int platform_types_configurations_count = 4; /*host, cpu, gpu, accel*/
    constexpr std::array<usm_thruth_table, platform_types_configurations_count> usm_target_table{ {
        { { usm_support_mode::direct,
            usm_support_mode::prohibited,
            usm_support_mode::shared } }, //host conf:  host, device, shared
        { { usm_support_mode::direct,
            usm_support_mode::prohibited,
            usm_support_mode::shared } }, //cpu conf:  host, device, shared
        { { usm_support_mode::prohibited,
            usm_support_mode::need_conversion,
            usm_support_mode::shared } }, //gpu conf:  host, device, shared
        { { usm_support_mode::prohibited,
            usm_support_mode::prohibited,
            usm_support_mode::shared } } //accel conf:  host, device, shared
    } };
    int platform_configuration_type_idx = get_platform_type_index(device);
    std::get<assoc_result_index::SUPPORT_MODE>(ret) =
        usm_target_table[platform_configuration_type_idx][utils::enum_to_underlying(pointer_type)];
    if (std::get<assoc_result_index::SUPPORT_MODE>(ret) == usm_support_mode::prohibited) {
        std::stringstream ss;
        ss << "Incompatible USM type requested: " << usm_to_string(pointer_type)
           << ", for ccl_device: " << std::to_string(platform_configuration_type_idx);
        std::get<assoc_result_index::ERROR_CAUSE>(ret) = ss.str();
    }
#else
    //TODO calls method `assoc` for ccl_device
#endif
    return ret;
}

std::string to_string(const assoc_retult& res)
{
    std::stringstream ss;
    ss << "Mem: " << std::get<assoc_result_index::POINTER_VALUE>(res) << ", is: "
       << to_string(std::get<assoc_result_index::SUPPORT_MODE>(res));
    const std::string& err_cause = std::get<assoc_result_index::ERROR_CAUSE>(res);
    if (!err_cause.empty())
    {
        ss << ", error cause: " << err_cause;
    }
    return ss.str();
}
} // namespace details
} // namespace native
