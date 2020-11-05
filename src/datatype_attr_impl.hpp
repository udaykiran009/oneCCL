#pragma once
#include "oneapi/ccl/types.hpp"
#include "oneapi/ccl/datatype_attr.hpp"

namespace ccl {

namespace v1 {

/**
 * datatype_attr attributes definition
 */
template <datatype_attr_id attrId, class Value>
CCL_API Value datatype_attr::set(const Value& v) {
    return get_impl()->set_attribute_value(
        v, detail::ccl_api_type_attr_traits<datatype_attr_id, attrId>{});
}

template <datatype_attr_id attrId>
CCL_API const typename detail::ccl_api_type_attr_traits<datatype_attr_id, attrId>::return_type&
datatype_attr::get() const {
    return get_impl()->get_attribute_value(
        detail::ccl_api_type_attr_traits<datatype_attr_id, attrId>{});
}

} // namespace v1

} // namespace ccl
