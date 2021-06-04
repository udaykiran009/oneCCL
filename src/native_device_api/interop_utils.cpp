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

} // namespace detail
} // namespace native
