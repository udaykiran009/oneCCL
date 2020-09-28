#pragma once

#include "oneapi/ccl/ccl_type_traits.hpp"
#include "common/log/log.hpp"

namespace ccl {
#ifdef CCL_ENABLE_SYCL

generic_device_context_type<cl_backend_type::dpcpp_sycl>::generic_device_context_type() {}
generic_device_context_type<cl_backend_type::dpcpp_sycl>::generic_device_context_type(ccl_native_t ctx)
        : context(ctx) {}

generic_device_context_type<cl_backend_type::dpcpp_sycl>::ccl_native_t&
generic_device_context_type<cl_backend_type::dpcpp_sycl>::get() noexcept {
    return const_cast<generic_device_context_type<cl_backend_type::dpcpp_sycl>::ccl_native_t&>(
        static_cast<const generic_device_context_type<cl_backend_type::dpcpp_sycl>*>(this)->get());
}

const generic_device_context_type<cl_backend_type::dpcpp_sycl>::ccl_native_t&
generic_device_context_type<cl_backend_type::dpcpp_sycl>::get() const noexcept {
    return context;
}

#else
#ifdef MULTI_GPU_SUPPORT

generic_device_context_type<cl_backend_type::l0>::generic_device_context_type() {}
generic_device_context_type<cl_backend_type::l0>::generic_device_context_type(handle_t ctx)
        : context() {
    //TODO context
    (void)ctx;

    throw;
}

generic_device_context_type<cl_backend_type::l0>::ccl_native_t
generic_device_context_type<cl_backend_type::l0>::get() noexcept {
    return /*const_cast<generic_device_context_type<cl_backend_type::l0>::ccl_native_t>*/ (
        static_cast<const generic_device_context_type<cl_backend_type::l0>*>(this)->get());
}

const generic_device_context_type<cl_backend_type::l0>::ccl_native_t&
generic_device_context_type<cl_backend_type::l0>::get() const noexcept {
    //TODO
    return context; //native::get_platform();
}
#else //MULTI_GPU_SUPPORT
generic_device_context_type<cl_backend_type::empty_backend>::generic_device_context_type(...) {}

generic_device_context_type<cl_backend_type::empty_backend>::ccl_native_t
generic_device_context_type<cl_backend_type::empty_backend>::get() noexcept {
    return /*const_cast<generic_device_context_type<cl_backend_type::l0>::ccl_native_t>*/ (
        static_cast<const generic_device_context_type<cl_backend_type::empty_backend>*>(this)->get());
}

const generic_device_context_type<cl_backend_type::empty_backend>::ccl_native_t&
generic_device_context_type<cl_backend_type::empty_backend>::get() const noexcept {
    //TODO
    return context; //native::get_platform();
}
#endif
#endif
} // namespace ccl
