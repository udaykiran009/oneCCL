#pragma once

#ifndef CCL_PRODUCT_FULL
#error "Do not include this file directly. Please include 'ccl.hpp'"
#endif

namespace ccl
{
namespace details
{

template <>
struct ccl_api_type_attr_traits<ccl_datatype_attributes, ccl_datatype_attributes::version> {
    using type = ccl::version;
    using return_type = type;
};

template <>
struct ccl_api_type_attr_traits<ccl_datatype_attributes, ccl_datatype_attributes::size> {
    using type = size_t;
    using return_type = type;
};

} // namespace details

} // namespace ccl
