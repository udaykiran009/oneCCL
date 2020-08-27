#pragma once
#include "ccl_types.hpp"
#include "ccl_types_policy.hpp"
#include "ccl_coll_attr_ids.hpp"
#include "ccl_coll_attr_ids_traits.hpp"

namespace ccl {
struct ccl_common_op_attr_impl_t
{
    ccl_common_op_attr_impl_t(const ccl_version_t& version);
    /**
     * `version` operations
     */
    using version_traits_t = details::ccl_api_type_attr_traits<common_op_attr_id, ccl::common_op_attr_id::version>;
    const typename version_traits_t::return_type&
        get_attribute_value(const version_traits_t& id) const;

    typename version_traits_t::return_type
        set_attribute_value(typename version_traits_t::type val, const version_traits_t& t);

    /**
     * `prolog_fn` operations
     */
    using prolog_fn_traits_t = details::ccl_api_type_attr_traits<common_op_attr_id, ccl::common_op_attr_id::prolog_fn>;
    const typename prolog_fn_traits_t::return_type&
        get_attribute_value(const prolog_fn_traits_t& id) const;

    typename prolog_fn_traits_t::return_type
        set_attribute_value(typename prolog_fn_traits_t::type val, const prolog_fn_traits_t& t);

    /**
     * `epilog_fn` operations
     */
    using epilog_fn_traits_t = details::ccl_api_type_attr_traits<common_op_attr_id, ccl::common_op_attr_id::epilog_fn>;
    const typename epilog_fn_traits_t::return_type&
        get_attribute_value(const epilog_fn_traits_t& id) const;

    typename epilog_fn_traits_t::return_type
        set_attribute_value(typename epilog_fn_traits_t::type val, const epilog_fn_traits_t& t);

    /**
     * `priority` operations
     */
    using priority_traits_t = details::ccl_api_type_attr_traits<common_op_attr_id, ccl::common_op_attr_id::priority>;
    const typename priority_traits_t::return_type&
        get_attribute_value(const priority_traits_t& id) const;

    typename priority_traits_t::return_type
        set_attribute_value(typename priority_traits_t::type val, const priority_traits_t& t);

    /**
     * `synchronous` operations
     */
    using synchronous_traits_t = details::ccl_api_type_attr_traits<common_op_attr_id, ccl::common_op_attr_id::synchronous>;
    const typename synchronous_traits_t::return_type&
        get_attribute_value(const synchronous_traits_t& id) const;

    typename synchronous_traits_t::return_type
        set_attribute_value(typename synchronous_traits_t::type val, const synchronous_traits_t& t);

    /**
     * `to_cache` operations
     */
    using to_cache_traits_t = details::ccl_api_type_attr_traits<common_op_attr_id, ccl::common_op_attr_id::to_cache>;
    const typename to_cache_traits_t::return_type&
        get_attribute_value(const to_cache_traits_t& id) const;

    typename to_cache_traits_t::return_type
        set_attribute_value(typename to_cache_traits_t::type val, const to_cache_traits_t& t);

    /**
     * `match_id` operations
     */
    using match_id_traits_t = details::ccl_api_type_attr_traits<common_op_attr_id, ccl::common_op_attr_id::match_id>;
    const typename match_id_traits_t::return_type&
        get_attribute_value(const match_id_traits_t& id) const;

    typename match_id_traits_t::return_type
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
    typename match_id_traits_t::return_type match_id;

protected:
    ccl_version_t library_version;
};
}
