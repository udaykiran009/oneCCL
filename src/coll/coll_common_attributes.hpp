#pragma once
#include "ccl_types.hpp"
namespace ccl {
struct ccl_common_op_attr_impl_t
{
    ccl_common_op_attr_impl_t(const ccl_version_t& version);
    /**
     * `version` operations
     */
    using version_traits_t = details::ccl_api_type_attr_traits<common_op_attr_id, ccl::common_op_attr_id::version>;
    const typename version_traits_t::type&
        get_attribute_value(const version_traits_t& id) const;

    typename version_traits_t::type
        set_attribute_value(typename version_traits_t::type val, const version_traits_t& t);

    /**
     * `prolog_fn` operations
     */
    using prolog_fn_traits_t = details::ccl_api_type_attr_traits<common_op_attr_id, ccl::common_op_attr_id::prolog_fn>;
    typename prolog_fn_traits_t::return_type
        get_attribute_value(const prolog_fn_traits_t& id) const;

    typename prolog_fn_traits_t::return_type
        set_attribute_value(typename prolog_fn_traits_t::type val, const prolog_fn_traits_t& t);

    /**
     * `epilog_fn` operations
     */
    using epilog_fn_traits_t = details::ccl_api_type_attr_traits<common_op_attr_id, ccl::common_op_attr_id::epilog_fn>;
    typename epilog_fn_traits_t::return_type
        get_attribute_value(const epilog_fn_traits_t& id) const;

    typename epilog_fn_traits_t::return_type
        set_attribute_value(typename epilog_fn_traits_t::type val, const epilog_fn_traits_t& t);

    /**
     * `priority` operations
     */
    using priority_traits_t = details::ccl_api_type_attr_traits<common_op_attr_id, ccl::common_op_attr_id::priority>;
    const typename priority_traits_t::type&
        get_attribute_value(const priority_traits_t& id) const;

    typename priority_traits_t::type
        set_attribute_value(typename priority_traits_t::type val, const priority_traits_t& t);

    /**
     * `synchronous` operations
     */
    using synchronous_traits_t = details::ccl_api_type_attr_traits<common_op_attr_id, ccl::common_op_attr_id::synchronous>;
    const typename synchronous_traits_t::type&
        get_attribute_value(const synchronous_traits_t& id) const;

    typename synchronous_traits_t::type
        set_attribute_value(typename synchronous_traits_t::type val, const synchronous_traits_t& t);

    /**
     * `to_cache` operations
     */
    using to_cache_traits_t = details::ccl_api_type_attr_traits<common_op_attr_id, ccl::common_op_attr_id::to_cache>;
    const typename to_cache_traits_t::type&
        get_attribute_value(const to_cache_traits_t& id) const;

    typename to_cache_traits_t::type
        set_attribute_value(typename to_cache_traits_t::type val, const to_cache_traits_t& t);

    /**
     * `match_id` operations
     */
    using match_id_traits_t = details::ccl_api_type_attr_traits<common_op_attr_id, ccl::common_op_attr_id::match_id>;
    const typename match_id_traits_t::type&
        get_attribute_value(const match_id_traits_t& id) const;

    typename match_id_traits_t::type
        set_attribute_value(typename match_id_traits_t::type val, const match_id_traits_t& t);


    typename ccl_common_op_attr_impl_t::prolog_fn_traits_t::return_type prologue_fn;
    typename ccl_common_op_attr_impl_t::epilog_fn_traits_t::return_type epilogue_fn;

    /* Priority for collective operation */
    size_t priority;

    /* Blocking/non-blocking */
    int synchronous;

    /* Persistent/non-persistent */
    int to_cache;


    /**
     * Id of the operation. If specified, new communicator will be created and collective
     * operations with the same @b match_id will be executed in the same order.
     */
    const char* match_id;

protected:
    ccl_version_t library_version;
};


/**
 * Definition
 */
ccl_common_op_attr_impl_t::ccl_common_op_attr_impl_t(const ccl_version_t& version) :
    library_version(version)
{
}

typename ccl_common_op_attr_impl_t::version_traits_t::type
ccl_common_op_attr_impl_t::set_attribute_value(typename version_traits_t::type val, const version_traits_t& t)
{
    (void)t;
    throw ccl_error("Set value for 'ccl::common_op_attr_id::version' is not allowed");
    return library_version;
}

const typename ccl_common_op_attr_impl_t::version_traits_t::type&
ccl_common_op_attr_impl_t::get_attribute_value(const version_traits_t& id) const
{
    return library_version;
}

/**
 * `prologue_fn` operations definitions
 */
typename ccl_common_op_attr_impl_t::prolog_fn_traits_t::return_type
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
typename ccl_common_op_attr_impl_t::epilog_fn_traits_t::return_type
ccl_common_op_attr_impl_t::get_attribute_value(const epilog_fn_traits_t& id) const
{
    return epilogue_fn;
}

typename ccl_common_op_attr_impl_t::epilog_fn_traits_t::return_type
ccl_common_op_attr_impl_t::set_attribute_value(typename epilog_fn_traits_t::type val, const epilog_fn_traits_t& t)
{
    auto old = epilogue_fn.get();
    epilogue_fn = typename epilog_fn_traits_t::return_type{val};
    return typename epilog_fn_traits_t::return_type{epilogue_fn};
}

/**
 * `priority` operations definitions
 */
const typename ccl_common_op_attr_impl_t::priority_traits_t::type&
ccl_common_op_attr_impl_t::get_attribute_value(const priority_traits_t& id) const
{
    return priority;
}

typename ccl_common_op_attr_impl_t::priority_traits_t::type
ccl_common_op_attr_impl_t::set_attribute_value(typename priority_traits_t::type val, const priority_traits_t& t)
{
    auto old = priority;
    std::swap(priority, val);
    return old;
}

/**
 * `synchronous` operations definitions
 */
const typename ccl_common_op_attr_impl_t::synchronous_traits_t::type&
ccl_common_op_attr_impl_t::get_attribute_value(const synchronous_traits_t& id) const
{
    return synchronous;
}

typename ccl_common_op_attr_impl_t::synchronous_traits_t::type
ccl_common_op_attr_impl_t::set_attribute_value(typename synchronous_traits_t::type val, const synchronous_traits_t& t)
{
    auto old = synchronous;
    std::swap(synchronous, val);
    return old;
}

/**
 * `to_cache` operations definitions
 */
const typename ccl_common_op_attr_impl_t::to_cache_traits_t::type&
ccl_common_op_attr_impl_t::get_attribute_value(const to_cache_traits_t& id) const
{
    return to_cache;
}

typename ccl_common_op_attr_impl_t::to_cache_traits_t::type
ccl_common_op_attr_impl_t::set_attribute_value(typename to_cache_traits_t::type val, const to_cache_traits_t& t)
{
    auto old = to_cache;
    std::swap(to_cache, val);
    return old;
}

/**
 * `match_id` operations definitions
 */
const typename ccl_common_op_attr_impl_t::match_id_traits_t::type&
ccl_common_op_attr_impl_t::get_attribute_value(const match_id_traits_t& id) const
{
    return match_id;
}

typename ccl_common_op_attr_impl_t::match_id_traits_t::type
ccl_common_op_attr_impl_t::set_attribute_value(typename match_id_traits_t::type val, const match_id_traits_t& t)
{
    auto old = match_id;
    std::swap(match_id, val);
    return old;
}
}
