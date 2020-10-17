#pragma once
#include "oneapi/ccl/ccl_types.hpp"
#include "oneapi/ccl/ccl_comm_attr.hpp"

namespace ccl {

/**
 * comm_attr attributes definition
 */
template <comm_attr_id attrId, class Value>
CCL_API Value comm_attr::set(const Value& v) {
    return get_impl()->set_attribute_value(
        v, detail::ccl_api_type_attr_traits<comm_attr_id, attrId>{});
}

template <comm_attr_id attrId>
CCL_API const typename detail::ccl_api_type_attr_traits<comm_attr_id, attrId>::type&
comm_attr::get() const {
    return get_impl()->get_attribute_value(
        detail::ccl_api_type_attr_traits<comm_attr_id, attrId>{});
}

template <comm_attr_id attrId>
CCL_API bool comm_attr::is_valid() const noexcept {
    return get_impl()->is_valid<attrId>();
}

} // namespace ccl
