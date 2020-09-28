#pragma once

#include "oneapi/ccl/ccl_type_traits.hpp"
#include "common/log/log.hpp"

namespace ccl {
#ifdef CCL_ENABLE_SYCL

generic_stream_type<cl_backend_type::dpcpp_sycl>::generic_stream_type(handle_t q) : queue(ev) {}

generic_stream_type<cl_backend_type::dpcpp_sycl>::ccl_native_t
generic_stream_type<cl_backend_type::dpcpp_sycl>::get() noexcept {
    return const_cast<generic_stream_type<cl_backend_type::dpcpp_sycl>::ccl_native_t>(
        static_cast<const generic_stream_type<cl_backend_type::dpcpp_sycl>*>(this)->get());
}

const generic_stream_type<cl_backend_type::dpcpp_sycl>::ccl_native_t&
generic_stream_type<cl_backend_type::dpcpp_sycl>::get() const noexcept {
    return queue;
}

#else
#ifdef MULTI_GPU_SUPPORT

generic_stream_type<cl_backend_type::l0>::generic_stream_type(handle_t q)
        : queue(/*TODO use ccl_device to create event*/) {}

generic_stream_type<cl_backend_type::l0>::ccl_native_t
generic_stream_type<cl_backend_type::l0>::get() noexcept {
    return const_cast<generic_stream_type<cl_backend_type::l0>::ccl_native_t>(
        static_cast<const generic_stream_type<cl_backend_type::l0>*>(this)->get());
}

const generic_stream_type<cl_backend_type::l0>::ccl_native_t&
generic_stream_type<cl_backend_type::l0>::get() const noexcept {
    return queue;
}
#else  //MULTI_GPU_SUPPORT

#endif
#endif
} // namespace ccl
