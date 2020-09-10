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
struct ccl_host_split_traits<ccl_comm_split_attributes, ccl_comm_split_attributes::version> {
    using type = ccl::version;
};

template <>
struct ccl_host_split_traits<ccl_comm_split_attributes, ccl_comm_split_attributes::color> {
    using type = int;
};

template <>
struct ccl_host_split_traits<ccl_comm_split_attributes, ccl_comm_split_attributes::group> {
    using type = ccl_group_split_type;
};



#ifdef MULTI_GPU_SUPPORT

/**
 * Device-specific traits
 */
template <class attr, attr id>
struct ccl_device_split_traits {};

template <>
struct ccl_device_split_traits<ccl_comm_split_attributes, ccl_comm_split_attributes::version> {
    using type = ccl::version;
};

template <>
struct ccl_device_split_traits<ccl_comm_split_attributes, ccl_comm_split_attributes::color> {
    using type = int;
};

template <>
struct ccl_device_split_traits<ccl_comm_split_attributes, ccl_comm_split_attributes::group> {
    using type = device_group_split_type;
};

#endif

} // namespace details

} // namespace ccl
