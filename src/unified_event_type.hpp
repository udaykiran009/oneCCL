#pragma once

#include "oneapi/ccl/ccl_type_traits.hpp"
#include "common/log/log.hpp"

namespace ccl
{
#ifdef CCL_ENABLE_SYCL

generic_event_type<CCL_ENABLE_SYCL_TRUE>::generic_event_type(handle_t ev) :
    event(ev)
{
}

generic_event_type<CCL_ENABLE_SYCL_TRUE>::ccl_native_t
    generic_event_type<CCL_ENABLE_SYCL_TRUE>::get() noexcept
{
    return const_cast<generic_event_type<CCL_ENABLE_SYCL_TRUE>::ccl_native_t>(static_cast<const generic_event_type<CCL_ENABLE_SYCL_TRUE>*>(this)->get());
}

const generic_event_type<CCL_ENABLE_SYCL_TRUE>::ccl_native_t&
    generic_event_type<CCL_ENABLE_SYCL_TRUE>::get() const noexcept
{
    return event;
}


#else

#ifdef MULTI_GPU_SUPPORT
generic_event_type<CCL_ENABLE_SYCL_FALSE>::generic_event_type(handle_t e) :
    event(/*TODO use ccl_device_context to create event*/)
{
}

generic_event_type<CCL_ENABLE_SYCL_FALSE>::ccl_native_t
    generic_event_type<CCL_ENABLE_SYCL_FALSE>::get() noexcept
{
    return const_cast<generic_event_type<CCL_ENABLE_SYCL_FALSE>::ccl_native_t>(static_cast<const generic_event_type<CCL_ENABLE_SYCL_FALSE>*>(this)->get());
}

const generic_event_type<CCL_ENABLE_SYCL_FALSE>::ccl_native_t&
    generic_event_type<CCL_ENABLE_SYCL_FALSE>::get() const noexcept
{
    return event;
}
#endif //MULTI_GPU_SUPPORT
#endif
}
