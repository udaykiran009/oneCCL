#pragma once

#if defined(MULTI_GPU_SUPPORT)
#include "oneapi/ccl/native_device_api/export_api.hpp"
#include "oneapi/ccl/native_device_api/l0/declarations.hpp"
#include "oneapi/ccl/type_traits.hpp"

#ifdef CCL_ENABLE_SYCL
#include <CL/sycl/backend/level_zero.hpp>
//static cl::sycl::vector_class<cl::sycl::device> gpu_sycl_devices;
#endif

#include "oneapi/ccl/native_device_api/l0/utils.hpp"

namespace native {
namespace detail {
static ccl_device_driver::device_ptr get_runtime_device_impl(const ccl::device_index_type& path) {
    return get_platform().get_device(path);
}

} // namespace detail

template <class DeviceType,
          typename std::enable_if<not std::is_same<typename std::remove_cv<DeviceType>::type,
                                                   ccl::device_index_type>::value,
                                  int>::type = 0>
/*CCL_API*/ ccl_device_driver::device_ptr get_runtime_device(const DeviceType& device) {
    static_assert(std::is_same<typename ccl::unified_device_type::ccl_native_t, DeviceType>::value,
                  "Unsupported 'DeviceType'");
    size_t driver_idx = 0; // limitation for OPENCL/SYCL
    size_t device_id = 0;
#ifdef CCL_ENABLE_SYCL
    device_id = native::detail::get_sycl_device_id(device);
#endif
    ccl::device_index_type path(driver_idx, device_id, ccl::unused_index_value);

    return detail::get_runtime_device_impl(path);
}

template <class DeviceType,
          typename std::enable_if<std::is_same<typename std::remove_cv<DeviceType>::type,
                                               ccl::device_index_type>::value,
                                  int>::type = 0>
/*CCL_API*/ ccl_device_driver::device_ptr get_runtime_device(const DeviceType& device) {
    return detail::get_runtime_device_impl(device);
}

template <class ContextType>
/*CCL_API*/ ccl_driver_context_ptr get_runtime_context(const ContextType& ctx) {
#ifdef CCL_ENABLE_SYCL
    static_assert(
        std::is_same<typename std::remove_cv<ContextType>::type, cl::sycl::context>::value,
        "Invalid ContextType");
    auto l0_handle_ptr = ctx.template get_native<cl::sycl::backend::level_zero>();
    if (!l0_handle_ptr) {
        throw std::runtime_error(std::string(__FUNCTION__) +
                                 " - failed for sycl context: handle is nullptr!");
    }
    auto& drivers = get_platform().get_drivers();
    assert(drivers.size() == 1 && "Only one driver supported for L0 at now");
    return drivers.begin()->second->create_context_from_handle(l0_handle_ptr);
#else
    return ctx;
#endif
}
} // namespace native

template native::ccl_device_driver::device_ptr native::get_runtime_device(
    const ccl::device_index_type& path);

template native::ccl_driver_context_ptr native::get_runtime_context(
    const ccl::unified_context_type::ccl_native_t& ctx);

#ifdef CCL_ENABLE_SYCL
template native::ccl_device_driver::device_ptr native::get_runtime_device(
    const cl::sycl::device& device);
#endif

#endif //#if defined(MULTI_GPU_SUPPORT) || defined(CCL_ENABLE_SYCL)
