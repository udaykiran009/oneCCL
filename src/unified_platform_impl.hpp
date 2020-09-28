#pragma once

#include "oneapi/ccl/ccl_type_traits.hpp"
#include "common/log/log.hpp"

namespace ccl {
#ifdef CCL_ENABLE_SYCL

generic_platform_type<cl_backend_type::dpcpp_sycl>::generic_platform_type(ccl_native_t pl)
        : platform(pl) {}

generic_platform_type<cl_backend_type::dpcpp_sycl>::ccl_native_t
generic_platform_type<cl_backend_type::dpcpp_sycl>::get() noexcept {
    return const_cast<generic_platform_type<cl_backend_type::dpcpp_sycl>::ccl_native_t>(
        static_cast<const generic_platform_type<cl_backend_type::dpcpp_sycl>*>(this)->get());
}

const generic_platform_type<cl_backend_type::dpcpp_sycl>::ccl_native_t&
generic_platform_type<cl_backend_type::dpcpp_sycl>::get() const noexcept {
    return platform;
}

#else
#ifdef MULTI_GPU_SUPPORT
generic_platform_type<cl_backend_type::l0>::ccl_native_t
generic_platform_type<cl_backend_type::l0>::get() noexcept {
    return const_cast<generic_platform_type<cl_backend_type::l0>::ccl_native_t>(
        static_cast<const generic_platform_type<cl_backend_type::l0>*>(this)->get());
}

const generic_platform_type<cl_backend_type::l0>::ccl_native_t&
generic_platform_type<cl_backend_type::l0>::get() const noexcept {
    return native::get_platform();
}
#endif //MULTI_GPU_SUPPORT
#endif
} // namespace ccl
