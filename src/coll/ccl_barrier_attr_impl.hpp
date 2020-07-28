#pragma once
#include "ccl_types.hpp"
namespace ccl {
struct ccl_barrier_attr_impl_t
{
    ccl_barrier_attr_impl_t(const ccl_version_t& version);
    /**
     * `version` operations
     */
    using version_traits_t = details::ccl_api_type_attr_traits<common_op_attr_id, ccl::common_op_attr_id::version>;
    const typename version_traits_t::type&
        get_attribute_value(const version_traits_t& id) const;

    typename version_traits_t::type
        set_attribute_value(typename version_traits_t::type val, const version_traits_t& t);

protected:
    ccl_version_t library_version;
};


/**
 * Definition
 */
ccl_barrier_attr_impl_t::ccl_barrier_attr_impl_t(const ccl_version_t& version) :
    library_version(version)
{
}

typename ccl_barrier_attr_impl_t::version_traits_t::type
ccl_barrier_attr_impl_t::set_attribute_value(typename version_traits_t::type val, const version_traits_t& t)
{
    (void)t;
    throw ccl_error("Set value for 'ccl::common_op_attr_id::version' is not allowed");
    return library_version;
}

const typename ccl_barrier_attr_impl_t::version_traits_t::type&
ccl_barrier_attr_impl_t::get_attribute_value(const version_traits_t& id) const
{
    return library_version;
}
}
