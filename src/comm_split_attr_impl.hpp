#pragma once
#include "oneapi/ccl/ccl_types.hpp"
#include "oneapi/ccl/ccl_comm_split_attr.hpp"

namespace ccl
{


/**
 * comm_split_attr attributes definition
 */
template<ccl_comm_split_attributes attrId, class Value>
CCL_API Value comm_split_attr::set(const Value& v)
{
    return get_impl()->set_attribute_value(v, details::ccl_host_split_traits<ccl_comm_split_attributes, attrId> {});
}

template <ccl_comm_split_attributes attrId>
CCL_API const typename details::ccl_host_split_traits<ccl_comm_split_attributes, attrId>::type& comm_split_attr::get() const
{
    return get_impl()->get_attribute_value(details::ccl_host_split_traits<ccl_comm_split_attributes, attrId>{});
}

template <ccl_comm_split_attributes attrId>
CCL_API bool comm_split_attr::is_valid() const noexcept
{
    return get_impl()->is_valid<attrId>();
}



#ifdef MULTI_GPU_SUPPORT

/**
 * device_comm_split_attr attributes definition
 */
template<ccl_comm_split_attributes attrId, class Value>
CCL_API Value device_comm_split_attr::set(const Value& v)
{
    return get_impl()->set_attribute_value(v, details::ccl_device_split_traits<ccl_comm_split_attributes, attrId> {});
}

template <ccl_comm_split_attributes attrId>
CCL_API const typename details::ccl_device_split_traits<ccl_comm_split_attributes, attrId>::type& device_comm_split_attr::get() const
{
    return get_impl()->get_attribute_value(details::ccl_device_split_traits<ccl_comm_split_attributes, attrId>{});
}

template <ccl_comm_split_attributes attrId>
CCL_API bool device_comm_split_attr::is_valid() const noexcept
{
    return get_impl()->is_valid<attrId>();
}

#endif

} // namespace ccl
