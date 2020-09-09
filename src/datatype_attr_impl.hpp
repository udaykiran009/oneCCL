#pragma once
#include "ccl_types.hpp"
#include "ccl_datatype_attr.hpp"

namespace ccl
{

/**
 * datatype_attr_t attributes definition
 */
template<ccl_datatype_attributes attrId, class Value>
CCL_API Value datatype_attr_t::set(const Value& v)
{
    return get_impl()->set_attribute_value(v, details::ccl_api_type_attr_traits<ccl_datatype_attributes, attrId> {});
}

template <ccl_datatype_attributes attrId>
CCL_API const typename details::ccl_api_type_attr_traits<ccl_datatype_attributes, attrId>::return_type& datatype_attr_t::get() const
{
    return get_impl()->get_attribute_value(details::ccl_api_type_attr_traits<ccl_datatype_attributes, attrId>{});
}

} // namespace ccl
