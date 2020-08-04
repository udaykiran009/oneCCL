#pragma once

#include "ccl_type_traits.hpp"
#include "common/log/log.hpp"

#ifdef CCL_ENABLE_SYCL


generic_platform_type<CCL_ENABLE_SYCL_TRUE>::generic_platform_type(native_reference_t pl) :
    platform(pl)
{
}

generic_platform_type<CCL_ENABLE_SYCL_TRUE>::native_reference_t
    generic_platform_type<CCL_ENABLE_SYCL_TRUE>::get() noexcept
{
    return const_cast<generic_platform_type<CCL_ENABLE_SYCL_TRUE>::native_reference_t>(static_cast<const generic_platform_type<CCL_ENABLE_SYCL_TRUE>*>(this)->get());
}

generic_platform_type<CCL_ENABLE_SYCL_TRUE>::native_const_reference_t
    generic_platform_type<CCL_ENABLE_SYCL_TRUE>::get() const noexcept
{
    return platform;
}


#else



generic_platform_type<CCL_ENABLE_SYCL_FALSE>::native_reference_t
    generic_platform_type<CCL_ENABLE_SYCL_FALSE>::get() noexcept
{
    return const_cast<generic_platform_type<CCL_ENABLE_SYCL_FALSE>::native_reference_t>(static_cast<const generic_platform_type<CCL_ENABLE_SYCL_FALSE>*>(this)->get());
}

generic_platform_type<CCL_ENABLE_SYCL_FALSE>::native_const_reference_t
    generic_platform_type<CCL_ENABLE_SYCL_FALSE>::get() const noexcept
{
    return native::get_platform();
}

#endif
