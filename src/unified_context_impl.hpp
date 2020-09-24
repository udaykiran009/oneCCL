#pragma once

#include "oneapi/ccl/ccl_type_traits.hpp"
#include "common/log/log.hpp"

namespace ccl {
#ifdef CCL_ENABLE_SYCL

generic_device_context_type<CCL_ENABLE_SYCL_TRUE>::generic_device_context_type() {}
generic_device_context_type<CCL_ENABLE_SYCL_TRUE>::generic_device_context_type(ccl_native_t ctx)
        : context(ctx) {}

generic_device_context_type<CCL_ENABLE_SYCL_TRUE>::ccl_native_t&
generic_device_context_type<CCL_ENABLE_SYCL_TRUE>::get() noexcept {
    return const_cast<generic_device_context_type<CCL_ENABLE_SYCL_TRUE>::ccl_native_t&>(
        static_cast<const generic_device_context_type<CCL_ENABLE_SYCL_TRUE>*>(this)->get());
}

const generic_device_context_type<CCL_ENABLE_SYCL_TRUE>::ccl_native_t&
generic_device_context_type<CCL_ENABLE_SYCL_TRUE>::get() const noexcept {
    return context;
}

#else
#ifdef MULTI_GPU_SUPPORT

generic_device_context_type<CCL_ENABLE_SYCL_FALSE>::generic_device_context_type() {}
generic_device_context_type<CCL_ENABLE_SYCL_FALSE>::generic_device_context_type(handle_t ctx)
        : context() {
    //TODO context
    (void)ctx;

    throw;
}

generic_device_context_type<CCL_ENABLE_SYCL_FALSE>::ccl_native_t
generic_device_context_type<CCL_ENABLE_SYCL_FALSE>::get() noexcept {
    return /*const_cast<generic_device_context_type<CCL_ENABLE_SYCL_FALSE>::ccl_native_t>*/ (
        static_cast<const generic_device_context_type<CCL_ENABLE_SYCL_FALSE>*>(this)->get());
}

const generic_device_context_type<CCL_ENABLE_SYCL_FALSE>::ccl_native_t&
generic_device_context_type<CCL_ENABLE_SYCL_FALSE>::get() const noexcept {
    //TODO
    return context; //native::get_platform();
}
#else //MULTI_GPU_SUPPORT
generic_device_context_type<CCL_ENABLE_SYCL_V>::generic_device_context_type(...) {}

generic_device_context_type<CCL_ENABLE_SYCL_V>::ccl_native_t
generic_device_context_type<CCL_ENABLE_SYCL_V>::get() noexcept {
    return /*const_cast<generic_device_context_type<CCL_ENABLE_SYCL_FALSE>::ccl_native_t>*/ (
        static_cast<const generic_device_context_type<CCL_ENABLE_SYCL_V>*>(this)->get());
}

const generic_device_context_type<CCL_ENABLE_SYCL_V>::ccl_native_t&
generic_device_context_type<CCL_ENABLE_SYCL_V>::get() const noexcept {
    //TODO
    return context; //native::get_platform();
}
#endif
#endif
} // namespace ccl
