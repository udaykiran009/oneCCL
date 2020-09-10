#pragma once
#include "oneapi/ccl/ccl_types.hpp"
#include "oneapi/ccl/ccl_types_policy.hpp"
#include "oneapi/ccl/ccl_coll_attr_ids.hpp"
#include "oneapi/ccl/ccl_coll_attr_ids_traits.hpp"
#include "coll/coll_common_attributes.hpp"
namespace ccl {

class ccl_sparse_allreduce_attr_impl_t : public ccl_common_op_attr_impl_t
{
public:
    using base_t = ccl_common_op_attr_impl_t;

    ccl_sparse_allreduce_attr_impl_t(const typename details::ccl_api_type_attr_traits<operation_attr_id, ccl::operation_attr_id::version>::type& version);

    using sparse_allreduce_completion_fn_traits = details::ccl_api_type_attr_traits<sparse_allreduce_attr_id, sparse_allreduce_attr_id::completion_fn>;
    typename sparse_allreduce_completion_fn_traits::return_type
        set_attribute_value(typename sparse_allreduce_completion_fn_traits::type val, const sparse_allreduce_completion_fn_traits& t);
    const typename sparse_allreduce_completion_fn_traits::return_type&
        get_attribute_value(const sparse_allreduce_completion_fn_traits& id) const;


    using sparse_allreduce_alloc_fn_traits = details::ccl_api_type_attr_traits<sparse_allreduce_attr_id, sparse_allreduce_attr_id::alloc_fn>;
    typename sparse_allreduce_alloc_fn_traits::return_type
        set_attribute_value(typename sparse_allreduce_alloc_fn_traits::type val, const sparse_allreduce_alloc_fn_traits& t);
    const typename sparse_allreduce_alloc_fn_traits::return_type&
        get_attribute_value(const sparse_allreduce_alloc_fn_traits& id) const;


    using sparse_allreduce_fn_ctx_traits = details::ccl_api_type_attr_traits<sparse_allreduce_attr_id, sparse_allreduce_attr_id::fn_ctx>;
    typename sparse_allreduce_fn_ctx_traits::return_type
        set_attribute_value(typename sparse_allreduce_fn_ctx_traits::type val, const sparse_allreduce_fn_ctx_traits& t);
    const typename sparse_allreduce_fn_ctx_traits::return_type&
        get_attribute_value(const sparse_allreduce_fn_ctx_traits& id) const;


    using sparse_coalesce_mode_traits = details::ccl_api_type_attr_traits<sparse_allreduce_attr_id, sparse_allreduce_attr_id::coalesce_mode>;
    typename sparse_coalesce_mode_traits::return_type
        set_attribute_value(typename sparse_coalesce_mode_traits::type val, const sparse_coalesce_mode_traits& t);
    const typename sparse_coalesce_mode_traits::return_type&
        get_attribute_value(const sparse_coalesce_mode_traits& id) const;

private:
    typename sparse_allreduce_completion_fn_traits::return_type reduction_fn_val{};
    typename sparse_allreduce_alloc_fn_traits::return_type alloc_fn_val{};
    typename sparse_allreduce_fn_ctx_traits::return_type fn_ctx_val = nullptr;
    typename sparse_coalesce_mode_traits::return_type mode_val = ccl::sparse_coalesce_mode::regular;
};
}
