#pragma once

#include "oneapi/ccl/ccl_type_traits.hpp"
#include "common/log/log.hpp"

namespace ccl {
#ifdef CCL_ENABLE_SYCL

generic_event_type<cl_backend_type::dpcpp_sycl>::generic_event_type(handle_t ev) : event(ev) {}

generic_event_type<cl_backend_type::dpcpp_sycl>::ccl_native_t
generic_event_type<cl_backend_type::dpcpp_sycl>::get() noexcept {
    return const_cast<generic_event_type<cl_backend_type::dpcpp_sycl>::ccl_native_t>(
        static_cast<const generic_event_type<cl_backend_type::dpcpp_sycl>*>(this)->get());
}

const generic_event_type<cl_backend_type::dpcpp_sycl>::ccl_native_t&
generic_event_type<cl_backend_type::dpcpp_sycl>::get() const noexcept {
    return event;
}

#else

#ifdef MULTI_GPU_SUPPORT
generic_event_type<cl_backend_type::l0>::generic_event_type(handle_t e)
        : event(/*TODO use ccl_device_context to create event*/) {}

generic_event_type<cl_backend_type::l0>::ccl_native_t
generic_event_type<cl_backend_type::l0>::get() noexcept {
    return const_cast<generic_event_type<cl_backend_type::l0>::ccl_native_t>(
        static_cast<const generic_event_type<cl_backend_type::l0>*>(this)->get());
}

const generic_event_type<cl_backend_type::l0>::ccl_native_t&
generic_event_type<cl_backend_type::l0>::get() const noexcept {
    return event;
}
#endif //MULTI_GPU_SUPPORT
#endif
} // namespace ccl
