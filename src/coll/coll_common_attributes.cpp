#include "coll/coll_common_attributes.hpp"

namespace ccl {
/**
 * Definition
 */
ccl_common_op_attr_impl_t::ccl_common_op_attr_impl_t(const ccl_version_t& version) :
    library_version(version)
{
}

typename ccl_common_op_attr_impl_t::version_traits_t::return_type
ccl_common_op_attr_impl_t::set_attribute_value(typename version_traits_t::type val, const version_traits_t& t)
{
    (void)t;
    throw ccl_error("Set value for 'ccl::common_op_attr_id::version' is not allowed");
    return library_version;
}

const typename ccl_common_op_attr_impl_t::version_traits_t::return_type&
ccl_common_op_attr_impl_t::get_attribute_value(const version_traits_t& id) const
{
    return library_version;
}

/**
 * `prologue_fn` operations definitions
 */
const typename ccl_common_op_attr_impl_t::prolog_fn_traits_t::return_type&
ccl_common_op_attr_impl_t::get_attribute_value(const prolog_fn_traits_t& id) const
{
    return prologue_fn;
}

typename ccl_common_op_attr_impl_t::prolog_fn_traits_t::return_type
ccl_common_op_attr_impl_t::set_attribute_value(typename prolog_fn_traits_t::type val, const prolog_fn_traits_t& t)
{
    auto old = prologue_fn.get();
    prologue_fn = typename prolog_fn_traits_t::return_type{val};
    return typename prolog_fn_traits_t::return_type{old};
}
/**
 * `epilog_fn` operations definitions
 */
const typename ccl_common_op_attr_impl_t::epilog_fn_traits_t::return_type&
ccl_common_op_attr_impl_t::get_attribute_value(const epilog_fn_traits_t& id) const
{
    return epilogue_fn;
}

typename ccl_common_op_attr_impl_t::epilog_fn_traits_t::return_type
ccl_common_op_attr_impl_t::set_attribute_value(typename epilog_fn_traits_t::type val, const epilog_fn_traits_t& t)
{
    auto old = epilogue_fn.get();
    epilogue_fn = typename epilog_fn_traits_t::return_type{val};
    return typename epilog_fn_traits_t::return_type{old};
}

/**
 * `priority` operations definitions
 */
const typename ccl_common_op_attr_impl_t::priority_traits_t::return_type&
ccl_common_op_attr_impl_t::get_attribute_value(const priority_traits_t& id) const
{
    return priority;
}

typename ccl_common_op_attr_impl_t::priority_traits_t::return_type
ccl_common_op_attr_impl_t::set_attribute_value(typename priority_traits_t::type val, const priority_traits_t& t)
{
    auto old = priority;
    std::swap(priority, val);
    return old;
}

/**
 * `synchronous` operations definitions
 */
const typename ccl_common_op_attr_impl_t::synchronous_traits_t::return_type&
ccl_common_op_attr_impl_t::get_attribute_value(const synchronous_traits_t& id) const
{
    return synchronous;
}

typename ccl_common_op_attr_impl_t::synchronous_traits_t::return_type
ccl_common_op_attr_impl_t::set_attribute_value(typename synchronous_traits_t::type val, const synchronous_traits_t& t)
{
    auto old = synchronous;
    std::swap(synchronous, val);
    return old;
}

/**
 * `to_cache` operations definitions
 */
const typename ccl_common_op_attr_impl_t::to_cache_traits_t::return_type&
ccl_common_op_attr_impl_t::get_attribute_value(const to_cache_traits_t& id) const
{
    return to_cache;
}

typename ccl_common_op_attr_impl_t::to_cache_traits_t::return_type
ccl_common_op_attr_impl_t::set_attribute_value(typename to_cache_traits_t::type val, const to_cache_traits_t& t)
{
    auto old = to_cache;
    std::swap(to_cache, val);
    return old;
}

/**
 * `match_id` operations definitions
 */
const typename ccl_common_op_attr_impl_t::match_id_traits_t::return_type&
ccl_common_op_attr_impl_t::get_attribute_value(const match_id_traits_t& id) const
{
    return match_id;
}

typename ccl_common_op_attr_impl_t::match_id_traits_t::return_type
ccl_common_op_attr_impl_t::set_attribute_value(typename match_id_traits_t::type val, const match_id_traits_t& t)
{
    auto old = match_id;
    std::swap(match_id, val);
    return old;
}
}
