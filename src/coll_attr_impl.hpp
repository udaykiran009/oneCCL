#pragma once
#include "ccl_coll_attr.hpp"
#include "coll/coll_attributes.hpp"
#include "coll/ccl_allgather_op_attr_impl.hpp"

namespace ccl
{
    /* TODO temporary function for UT compilation: would be part of ccl::environment in final*/
template <class coll_attribute_type,
          class attr_value_pair... prop_ids>
coll_attribute_type create_coll_attr(attr_value_pair&&...avps)
{
    ccl_version_t ret;
    return coll_attribute_type(new coll_attribute_type::impl_t(ccl_common_op_attr_impl_t(), ret));
}

/**
 *
 */
CCL_API allgatherv_attr_t::allgatherv_attr_t(const allgatherv_attr_t& src) :
        allgatherv_attr_t(src.get_version(),
                          *(src.pimpl))
{
}

CCL_API allgatherv_attr_t::allgatherv_attr_t(const ccl_version_t& library_version,
                                     const ccl_host_comm_attr_t &core,
                                     ccl_version_t api_version) :
        pimpl(new impl_t(core, library_version))
{
    if (api_version.major != library_version.major)
    {
        throw ccl::ccl_error(std::string(__FUNCTION__) + "API & library versions are compatible: API is " +
                             std::to_string(api_version.major) +
                             ", library is: " + std::to_string(library_version.major));
    }
}

CCL_API allgatherv_attr_t::~allgatherv_attr_t() noexcept
{
}

template<int attrId,
             class Value,
             typename T>
CCL_API Value allgatherv_attr_t::set_value(const Value& v)
{
    return pimpl->set_attribute_value(v);
}

template<int attrId>
CCL_API const typename allgatherv_attr_traits<attrId>::type& allgatherv_attr_t::get_value() const
{
    //allgatherv_op_attr_id
    return pimpl->get_attribute_value(
            std::integral_constant<allgatherv_op_attr_id, attrId> {});
}
}
