#pragma once

#ifndef CCL_PRODUCT_FULL
#error "Do not include this file directly. Please include 'ccl.hpp'"
#endif

namespace ccl {
namespace details {

template <>
struct ccl_api_type_attr_traits<datatype_attr_id, datatype_attr_id::version> {
    using type = ccl::library_version;
    using return_type = type;
};

template <>
struct ccl_api_type_attr_traits<datatype_attr_id, datatype_attr_id::size> {
    using type = size_t;
    using return_type = type;
};

} // namespace details

} // namespace ccl
