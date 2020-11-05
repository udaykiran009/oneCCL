#pragma once
#include "oneapi/ccl/types.hpp"
#include "oneapi/ccl/kvs_attr.hpp"

namespace ccl {

namespace v1 {

/**
 * kvs_attr attributes definition
 */
template <kvs_attr_id attrId, class Value>
CCL_API Value kvs_attr::set(const Value& v) {
    return get_impl()->set_attribute_value(v,
                                           detail::ccl_api_type_attr_traits<kvs_attr_id, attrId>{});
}

template <kvs_attr_id attrId>
CCL_API const typename detail::ccl_api_type_attr_traits<kvs_attr_id, attrId>::type& kvs_attr::get()
    const {
    return get_impl()->get_attribute_value(detail::ccl_api_type_attr_traits<kvs_attr_id, attrId>{});
}

template <kvs_attr_id attrId>
CCL_API bool kvs_attr::is_valid() const noexcept {
    return get_impl()->is_valid<attrId>();
}

} // namespace v1

} // namespace ccl
