#pragma once
#include "ccl_types.hpp"
#include "ccl_types_policy.hpp"
#include "ccl_coll_attr_ids.hpp"
#include "ccl_coll_attr_ids_traits.hpp"
#include "coll/coll_common_attributes.hpp"
namespace ccl {

class ccl_allgather_op_attr_impl_t : public ccl_common_op_attr_impl_t
{
public:
    using base_t = ccl_common_op_attr_impl_t;

    ccl_allgather_op_attr_impl_t(const base_t& base);
    ccl_allgather_op_attr_impl_t(const typename details::ccl_api_type_attr_traits<common_op_attr_id, ccl::common_op_attr_id::version>::type& version);
    ccl_allgather_op_attr_impl_t(const ccl_allgather_op_attr_impl_t& src);

    using vector_buf_traits_t = details::ccl_api_type_attr_traits<allgatherv_op_attr_id, allgatherv_op_attr_id::vector_buf>;
    typename vector_buf_traits_t::type
        set_attribute_value(typename vector_buf_traits_t::type val, const vector_buf_traits_t& t);

    const typename vector_buf_traits_t::type&
        get_attribute_value(const vector_buf_traits_t& id) const;

private:
    typename vector_buf_traits_t::type vector_buf_id_val = 0;
};
}
