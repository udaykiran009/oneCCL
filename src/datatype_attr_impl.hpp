#pragma once
#include "oneapi/ccl/ccl_types.hpp"
#include "oneapi/ccl/ccl_datatype_attr.hpp"

namespace ccl {

/**
 * datatype_attr attributes definition
 */
template <datatype_attr_id attrId, class Value>
CCL_API Value datatype_attr::set(const Value& v) {
    return get_impl()->set_attribute_value(
        v, details::ccl_api_type_attr_traits<datatype_attr_id, attrId>{});
}

template <datatype_attr_id attrId>
CCL_API const typename details::ccl_api_type_attr_traits<datatype_attr_id, attrId>::return_type&
datatype_attr::get() const {
    return get_impl()->get_attribute_value(
        details::ccl_api_type_attr_traits<datatype_attr_id, attrId>{});
}

} // namespace ccl
