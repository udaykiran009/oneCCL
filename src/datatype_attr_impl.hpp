#pragma once
#include "oneapi/ccl/ccl_types.hpp"
#include "oneapi/ccl/ccl_datatype_attr.hpp"

namespace ccl
{

/**
 * datatype_attr attributes definition
 */
template<ccl_datatype_attributes attrId, class Value>
CCL_API Value datatype_attr::set(const Value& v)
{
    return get_impl()->set_attribute_value(v, details::ccl_api_type_attr_traits<ccl_datatype_attributes, attrId> {});
}

template <ccl_datatype_attributes attrId>
CCL_API const typename details::ccl_api_type_attr_traits<ccl_datatype_attributes, attrId>::return_type& datatype_attr::get() const
{
    return get_impl()->get_attribute_value(details::ccl_api_type_attr_traits<ccl_datatype_attributes, attrId>{});
}

} // namespace ccl
