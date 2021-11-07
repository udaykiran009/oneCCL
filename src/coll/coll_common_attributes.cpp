#include "coll/coll_common_attributes.hpp"

namespace ccl {
/**
 * Definition
 */
ccl_operation_attr_impl_t::ccl_operation_attr_impl_t(const ccl::library_version& version)
        : version(version) {}

typename ccl_operation_attr_impl_t::version_traits_t::return_type
ccl_operation_attr_impl_t::set_attribute_value(typename version_traits_t::type val,
                                               const version_traits_t& t) {
    (void)t;
    throw ccl::exception("Set value for 'ccl::operation_attr_id::version' is not allowed");
    return version;
}

const typename ccl_operation_attr_impl_t::version_traits_t::return_type&
ccl_operation_attr_impl_t::get_attribute_value(const version_traits_t& id) const {
    return version;
}

/**
 * `priority` operations definitions
 */
const typename ccl_operation_attr_impl_t::priority_traits_t::return_type&
ccl_operation_attr_impl_t::get_attribute_value(const priority_traits_t& id) const {
    return priority;
}

typename ccl_operation_attr_impl_t::priority_traits_t::return_type
ccl_operation_attr_impl_t::set_attribute_value(typename priority_traits_t::type val,
                                               const priority_traits_t& t) {
    auto old = priority;
    std::swap(priority, val);
    return old;
}

/**
 * `synchronous` operations definitions
 */
const typename ccl_operation_attr_impl_t::synchronous_traits_t::return_type&
ccl_operation_attr_impl_t::get_attribute_value(const synchronous_traits_t& id) const {
    return synchronous;
}

typename ccl_operation_attr_impl_t::synchronous_traits_t::return_type
ccl_operation_attr_impl_t::set_attribute_value(typename synchronous_traits_t::type val,
                                               const synchronous_traits_t& t) {
    auto old = synchronous;
    std::swap(synchronous, val);
    return old;
}

/**
 * `to_cache` operations definitions
 */
const typename ccl_operation_attr_impl_t::to_cache_traits_t::return_type&
ccl_operation_attr_impl_t::get_attribute_value(const to_cache_traits_t& id) const {
    return to_cache;
}

typename ccl_operation_attr_impl_t::to_cache_traits_t::return_type
ccl_operation_attr_impl_t::set_attribute_value(typename to_cache_traits_t::type val,
                                               const to_cache_traits_t& t) {
    auto old = to_cache;
    std::swap(to_cache, val);
    return old;
}

/**
 * `match_id` operations definitions
 */
const typename ccl_operation_attr_impl_t::match_id_traits_t::return_type&
ccl_operation_attr_impl_t::get_attribute_value(const match_id_traits_t& id) const {
    return match_id;
}

typename ccl_operation_attr_impl_t::match_id_traits_t::return_type
ccl_operation_attr_impl_t::set_attribute_value(typename match_id_traits_t::type val,
                                               const match_id_traits_t& t) {
    auto old = match_id;
    std::swap(match_id, val);
    return old;
}
} // namespace ccl
