#pragma once
#include "ccl_coll_attr_ids.hpp"
#include "ccl_coll_attr_ids_traits.hpp"
#include "coll/coll_common_attributes.hpp"
namespace ccl {

class ccl_reduce_scatter_op_attr_impl_t : public ccl_common_op_attr_impl_t
{
public:
    using base_t = ccl_common_op_attr_impl_t;

    ccl_reduce_scatter_op_attr_impl_t(const typename details::ccl_api_type_attr_traits<common_op_attr_id, ccl::common_op_attr_id::version>::type& version);

    using reduction_fn_traits_t = details::ccl_api_type_attr_traits<reduce_scatter_op_attr_id, reduce_scatter_op_attr_id::reduction_fn>;
    typename reduction_fn_traits_t::return_type
        set_attribute_value(typename reduction_fn_traits_t::type val, const reduction_fn_traits_t& t);

    const typename reduction_fn_traits_t::return_type&
        get_attribute_value(const reduction_fn_traits_t& id) const;

private:
    typename reduction_fn_traits_t::return_type reduction_fn_val;
};



ccl_reduce_scatter_op_attr_impl_t::ccl_reduce_scatter_op_attr_impl_t(const typename ccl_common_op_attr_impl_t::version_traits_t::type& version) :
        base_t(version)
{
}

typename ccl_reduce_scatter_op_attr_impl_t::reduction_fn_traits_t::return_type
ccl_reduce_scatter_op_attr_impl_t::set_attribute_value(typename reduction_fn_traits_t::type val, const reduction_fn_traits_t&t)
{
    auto old = reduction_fn_val;
    reduction_fn_val = typename reduction_fn_traits_t::return_type{val};
    return typename reduction_fn_traits_t::return_type{old};
}

const typename ccl_reduce_scatter_op_attr_impl_t::reduction_fn_traits_t::return_type&
ccl_reduce_scatter_op_attr_impl_t::get_attribute_value(const reduction_fn_traits_t& id) const
{
    return reduction_fn_val;
}
}
