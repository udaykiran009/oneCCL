#pragma once

#include "oneapi/ccl/ccl_type_traits.hpp"
#include "common/log/log.hpp"

namespace ccl
{
#ifdef CCL_ENABLE_SYCL

generic_stream_type<CCL_ENABLE_SYCL_TRUE>::generic_stream_type(handle_t q) :
    queue(ev)
{
}

generic_stream_type<CCL_ENABLE_SYCL_TRUE>::ccl_native_t
    generic_stream_type<CCL_ENABLE_SYCL_TRUE>::get() noexcept
{
    return const_cast<generic_stream_type<CCL_ENABLE_SYCL_TRUE>::ccl_native_t>(static_cast<const generic_stream_type<CCL_ENABLE_SYCL_TRUE>*>(this)->get());
}

const generic_stream_type<CCL_ENABLE_SYCL_TRUE>::ccl_native_t&
    generic_stream_type<CCL_ENABLE_SYCL_TRUE>::get() const noexcept
{
    return queue;
}


#else
#ifdef MULTI_GPU_SUPPORT

generic_stream_type<CCL_ENABLE_SYCL_FALSE>::generic_stream_type(handle_t q) :
    queue(/*TODO use ccl_device to create event*/)
{
}

generic_stream_type<CCL_ENABLE_SYCL_FALSE>::ccl_native_t
    generic_stream_type<CCL_ENABLE_SYCL_FALSE>::get() noexcept
{
    return const_cast<generic_stream_type<CCL_ENABLE_SYCL_FALSE>::ccl_native_t>(static_cast<const generic_stream_type<CCL_ENABLE_SYCL_FALSE>*>(this)->get());
}

const generic_stream_type<CCL_ENABLE_SYCL_FALSE>::ccl_native_t&
    generic_stream_type<CCL_ENABLE_SYCL_FALSE>::get() const noexcept
{
    return queue;
}
#endif MULTI_GPU_SUPPORT
#endif
}
