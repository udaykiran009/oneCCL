#pragma once
#include "oneapi/ccl/ccl_types.hpp"
#include "oneapi/ccl/ccl_init_attr_ids.hpp"
#include "oneapi/ccl/ccl_init_attr_ids_traits.hpp"
#include "oneapi/ccl/ccl_init_attr.hpp"

namespace ccl {

class init_attr_impl {
public:
    /**
     * `version` operations
     */
    using version_traits_t =
        detail::ccl_api_type_attr_traits<init_attr_id, init_attr_id::version>;

    const typename version_traits_t::return_type& get_attribute_value(
        const version_traits_t& id) const {
        return version;
    }

    typename version_traits_t::return_type set_attribute_value(typename version_traits_t::type val,
                                                               const version_traits_t& t) {
        (void)t;
        throw ccl::exception("Set value for 'ccl::init_attr_id::version' is not allowed");
        return version;
    }

    init_attr_impl(const typename version_traits_t::return_type& version)
            : version(version) {}

protected:
    typename version_traits_t::return_type version;
};



/**
 * init_attr attributes definition
 */
template <init_attr_id attrId, class Value>
Value init_attr::set(const Value& v) {
    return get_impl()->set_attribute_value(
        v, detail::ccl_api_type_attr_traits<init_attr_id, attrId>{});
}

template <init_attr_id attrId>
const typename detail::ccl_api_type_attr_traits<init_attr_id, attrId>::return_type&
init_attr::get() const {
    return get_impl()->get_attribute_value(
        detail::ccl_api_type_attr_traits<init_attr_id, attrId>{});
}

} // namespace ccl
