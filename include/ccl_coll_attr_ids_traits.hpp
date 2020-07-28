#pragma once

#ifndef CCL_PRODUCT_FULL
#error "Do not include this file directly. Please include 'ccl.hpp'"
#endif

namespace ccl {

namespace details {
template<class T>
class function_holder
{
public:
    function_holder(T in = nullptr) : val(in) {}
    T get() const { return val; }
private:
    T val;
};
/**
 * Traits for common attributes specializations
 */
template <>
struct ccl_api_type_attr_traits<common_op_attr_id, common_op_attr_id::version> {
    using type = ccl_version_t;
    using return_type = type;
};

template <>
struct ccl_api_type_attr_traits<common_op_attr_id, common_op_attr_id::prolog_fn> {
    using type = ccl_prologue_fn_t;
    using return_type = function_holder<type>;
};

template <>
struct ccl_api_type_attr_traits<common_op_attr_id, common_op_attr_id::epilog_fn> {
    using type = ccl_epilogue_fn_t;
    using return_type = function_holder<type>;
};

template <>
struct ccl_api_type_attr_traits<common_op_attr_id, common_op_attr_id::priority> {
    using type = size_t;
    using return_type = type;
};

template <>
struct ccl_api_type_attr_traits<common_op_attr_id, common_op_attr_id::synchronous> {
    using type = int;
    using return_type = type;
};

template <>
struct ccl_api_type_attr_traits<common_op_attr_id, common_op_attr_id::to_cache> {
    using type = int;
    using return_type = type;
};

template <>
struct ccl_api_type_attr_traits<common_op_attr_id, common_op_attr_id::match_id> {
    using type = const char*;
    using return_type = type;
};

/**
 * Traits specialization for allgatherv op attributes
 */
template <>
struct ccl_api_type_attr_traits<allgatherv_op_attr_id, allgatherv_op_attr_id::vector_buf> {
    using type = int;
    using return_type = type;
};

/**
 * Traits specialization for allreduce op attributes
 */
template <>
struct ccl_api_type_attr_traits<allreduce_op_attr_id, allreduce_op_attr_id::reduction_fn> {
    using type = ccl_reduction_fn_t;
    using return_type = function_holder<type>;
};

/**
 * Traits specialization for alltoall op attributes
 */

/**
 * Traits specialization for alltoallv op attributes
 */

/**
 * Traits specialization for bcast op attributes
 */

/**
 * Traits specialization for reduce op attributes
 */
template <>
struct ccl_api_type_attr_traits<reduce_op_attr_id, reduce_op_attr_id::reduction_fn> {
    using type = ccl_reduction_fn_t;
    using return_type = function_holder<type>;
};

/**
 * Traits specialization for reduce_scatter op attributes
 */
template <>
struct ccl_api_type_attr_traits<reduce_scatter_op_attr_id, reduce_scatter_op_attr_id::reduction_fn> {
    using type = ccl_reduction_fn_t;
    using return_type = function_holder<type>;
};

/**
 * Traits specialization for sparse_allreduce op attributes
 */
template <>
struct ccl_api_type_attr_traits<sparse_allreduce_op_attr_id, sparse_allreduce_op_attr_id::sparse_allreduce_completion_fn> {
    using type = ccl_sparse_allreduce_completion_fn_t;
    using return_type = function_holder<type>;
};
template <>
struct ccl_api_type_attr_traits<sparse_allreduce_op_attr_id, sparse_allreduce_op_attr_id::sparse_allreduce_alloc_fn> {
    using type = ccl_sparse_allreduce_alloc_fn_t;
    using return_type = function_holder<type>;
};
template <>
struct ccl_api_type_attr_traits<sparse_allreduce_op_attr_id, sparse_allreduce_op_attr_id::sparse_allreduce_fn_ctx> {
    using type = const void*;
    using return_type = function_holder<type>;
};
template <>
struct ccl_api_type_attr_traits<sparse_allreduce_op_attr_id, sparse_allreduce_op_attr_id::sparse_coalesce_mode> {
    using type = ccl_sparse_coalesce_mode_t;
    using return_type = type;
};
}
}
