#include "coll/ccl_reduce_op_attr.hpp"

namespace ccl {

ccl_reduce_attr_impl_t::ccl_reduce_attr_impl_t(const typename ccl_operation_attr_impl_t::version_traits_t::type& version) :
        base_t(version)
{
}

typename ccl_reduce_attr_impl_t::reduction_fn_traits_t::return_type
ccl_reduce_attr_impl_t::set_attribute_value(typename reduction_fn_traits_t::type val, const reduction_fn_traits_t&t)
{
    auto old = reduction_fn_val;
    reduction_fn_val = typename reduction_fn_traits_t::return_type{val};
    return typename reduction_fn_traits_t::return_type{old};

}

const typename ccl_reduce_attr_impl_t::reduction_fn_traits_t::return_type&
ccl_reduce_attr_impl_t::get_attribute_value(const reduction_fn_traits_t& id) const
{
    return reduction_fn_val;
}
}
