#pragma once

#include "oneapi/ccl/ccl_type_traits.hpp"
#include "common/log/log.hpp"

namespace ccl
{
#ifdef CCL_ENABLE_SYCL


generic_platform_type<CCL_ENABLE_SYCL_TRUE>::generic_platform_type(ccl_native_t pl) :
    platform(pl)
{
}

generic_platform_type<CCL_ENABLE_SYCL_TRUE>::ccl_native_t
    generic_platform_type<CCL_ENABLE_SYCL_TRUE>::get() noexcept
{
    return const_cast<generic_platform_type<CCL_ENABLE_SYCL_TRUE>::ccl_native_t>(static_cast<const generic_platform_type<CCL_ENABLE_SYCL_TRUE>*>(this)->get());
}

const generic_platform_type<CCL_ENABLE_SYCL_TRUE>::ccl_native_t&
    generic_platform_type<CCL_ENABLE_SYCL_TRUE>::get() const noexcept
{
    return platform;
}


#else
#ifdef MULTI_GPU_SUPPORT
generic_platform_type<CCL_ENABLE_SYCL_FALSE>::ccl_native_t
    generic_platform_type<CCL_ENABLE_SYCL_FALSE>::get() noexcept
{
    return const_cast<generic_platform_type<CCL_ENABLE_SYCL_FALSE>::ccl_native_t>(static_cast<const generic_platform_type<CCL_ENABLE_SYCL_FALSE>*>(this)->get());
}

const generic_platform_type<CCL_ENABLE_SYCL_FALSE>::ccl_native_t&
    generic_platform_type<CCL_ENABLE_SYCL_FALSE>::get() const noexcept
{
    return native::get_platform();
}
#endif //MULTI_GPU_SUPPORT
#endif
}
