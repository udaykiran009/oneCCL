#pragma once
#include "ccl_types.hpp"
#include "ccl_types_policy.hpp"
#include "ccl_coll_attr_ids.hpp"
#include "ccl_coll_attr_ids_traits.hpp"
#include "coll/coll_common_attributes.hpp"
namespace ccl {

class ccl_allreduce_op_attr_impl_t : public ccl_common_op_attr_impl_t
{
public:
    using base_t = ccl_common_op_attr_impl_t;

    ccl_allreduce_op_attr_impl_t(const typename details::ccl_api_type_attr_traits<common_op_attr_id, ccl::common_op_attr_id::version>::type& version);

    using reduction_fn_traits_t = details::ccl_api_type_attr_traits<allreduce_op_attr_id, allreduce_op_attr_id::reduction_fn>;
    typename reduction_fn_traits_t::return_type
        set_attribute_value(typename reduction_fn_traits_t::type val, const reduction_fn_traits_t& t);

    const typename reduction_fn_traits_t::return_type&
        get_attribute_value(const reduction_fn_traits_t& id) const;

private:
    typename reduction_fn_traits_t::return_type reduction_fn_val{};
};

}
