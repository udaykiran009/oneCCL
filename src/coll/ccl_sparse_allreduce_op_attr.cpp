#include "coll/ccl_sparse_allreduce_op_attr.hpp"

namespace ccl {

ccl_sparse_allreduce_attr_impl_t::ccl_sparse_allreduce_attr_impl_t(
    const typename ccl_operation_attr_impl_t::version_traits_t::type& version)
        : base_t(version) {}

typename ccl_sparse_allreduce_attr_impl_t::sparse_allreduce_completion_fn_traits::return_type
ccl_sparse_allreduce_attr_impl_t::set_attribute_value(
    typename sparse_allreduce_completion_fn_traits::type val,
    const sparse_allreduce_completion_fn_traits& t) {
    auto old = completion_fn_val;
    completion_fn_val = typename sparse_allreduce_completion_fn_traits::return_type{ val };
    return typename sparse_allreduce_completion_fn_traits::return_type{ old };
}

const typename ccl_sparse_allreduce_attr_impl_t::sparse_allreduce_completion_fn_traits::return_type&
ccl_sparse_allreduce_attr_impl_t::get_attribute_value(
    const sparse_allreduce_completion_fn_traits& id) const {
    return completion_fn_val;
}

typename ccl_sparse_allreduce_attr_impl_t::sparse_allreduce_alloc_fn_traits::return_type
ccl_sparse_allreduce_attr_impl_t::set_attribute_value(
    typename sparse_allreduce_alloc_fn_traits::type val,
    const sparse_allreduce_alloc_fn_traits& t) {
    auto old = alloc_fn_val;
    alloc_fn_val = typename sparse_allreduce_alloc_fn_traits::return_type{ val };
    return typename sparse_allreduce_alloc_fn_traits::return_type{ old };
}

const typename ccl_sparse_allreduce_attr_impl_t::sparse_allreduce_alloc_fn_traits::return_type&
ccl_sparse_allreduce_attr_impl_t::get_attribute_value(
    const sparse_allreduce_alloc_fn_traits& id) const {
    return alloc_fn_val;
}

typename ccl_sparse_allreduce_attr_impl_t::sparse_allreduce_fn_ctx_traits::return_type
ccl_sparse_allreduce_attr_impl_t::set_attribute_value(
    typename sparse_allreduce_fn_ctx_traits::type val,
    const sparse_allreduce_fn_ctx_traits& t) {
    auto old = fn_ctx_val;
    fn_ctx_val = typename sparse_allreduce_fn_ctx_traits::return_type{ val };
    return typename sparse_allreduce_fn_ctx_traits::return_type{ old };
}

const typename ccl_sparse_allreduce_attr_impl_t::sparse_allreduce_fn_ctx_traits::return_type&
ccl_sparse_allreduce_attr_impl_t::get_attribute_value(
    const sparse_allreduce_fn_ctx_traits& id) const {
    return fn_ctx_val;
}

typename ccl_sparse_allreduce_attr_impl_t::sparse_coalesce_mode_traits::return_type
ccl_sparse_allreduce_attr_impl_t::set_attribute_value(
    typename sparse_coalesce_mode_traits::type val,
    const sparse_coalesce_mode_traits& t) {
    auto old = mode_val;
    std::swap(mode_val, val);
    return typename sparse_coalesce_mode_traits::return_type{ old };
}

const typename ccl_sparse_allreduce_attr_impl_t::sparse_coalesce_mode_traits::return_type&
ccl_sparse_allreduce_attr_impl_t::get_attribute_value(const sparse_coalesce_mode_traits& id) const {
    return mode_val;
}

} // namespace ccl
