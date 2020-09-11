#pragma once

#ifndef CCL_PRODUCT_FULL
#error "Do not include this file directly. Please include 'ccl.hpp'"
#endif

namespace ccl
{

namespace details
{

/**
 * Host-specific traits
 */
template <class attr, attr id>
struct ccl_host_split_traits {};

template <>
struct ccl_host_split_traits<comm_split_attr_id, comm_split_attr_id::version> {
    using type = ccl::library_version;
};

template <>
struct ccl_host_split_traits<comm_split_attr_id, comm_split_attr_id::color> {
    using type = int;
};

template <>
struct ccl_host_split_traits<comm_split_attr_id, comm_split_attr_id::group> {
    using type = ccl_group_split_type;
};



#if defined(MULTI_GPU_SUPPORT) || defined(CCL_ENABLE_SYCL)

/**
 * Device-specific traits
 */
template <class attr, attr id>
struct ccl_device_split_traits {};

template <>
struct ccl_device_split_traits<comm_split_attr_id, comm_split_attr_id::version> {
    using type = ccl::library_version;
};

template <>
struct ccl_device_split_traits<comm_split_attr_id, comm_split_attr_id::color> {
    using type = int;
};

template <>
struct ccl_device_split_traits<comm_split_attr_id, comm_split_attr_id::group> {
    using type = device_group_split_type;
};

#endif //#if defined(MULTI_GPU_SUPPORT) || defined(CCL_ENABLE_SYCL)

} // namespace details

} // namespace ccl
