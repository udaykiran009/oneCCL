#pragma once

#ifndef CCL_PRODUCT_FULL
#error "Do not include this file directly. Please include 'ccl.hpp'"
#endif

namespace ccl {
/**
 *  traits for common attributes
 */
template <int attrId>
struct common_op_attr_traits {};

/**
 * Traits for common attributes specializations
 */
template <>
struct common_op_attr_traits<common_op_attr_id::version_id> {
    using type = size_t;
};
template <>
struct common_op_attr_traits<common_op_attr_id::prolog_fn_id> {
    using type = ccl_prologue_fn_t;
};
template <>
struct common_op_attr_traits<common_op_attr_id::epilog_fn_id> {
    using type = ccl_epilogue_fn_t;
};
template <>
struct common_op_attr_traits<common_op_attr_id::priority_id> {
    using type = size_t;
};
template <>
struct common_op_attr_traits<common_op_attr_id::synchronous_id> {
    using type = int;
};
template <>
struct common_op_attr_traits<common_op_attr_id::to_cache_id> {
    using type = int;
};
template <>
struct common_op_attr_traits<common_op_attr_id::match_id> {
    using type = const char*;
};

/**
 *  Traits for collective attributes
 */
template <int attrId>
struct allgatherv_attr_traits : public common_op_attr_traits<attrId> {};

template <int attrId>
struct allreduce_attr_traits : public common_op_attr_traits<attrId> {};

template <int attrId>
struct alltoall_attr_traits : public common_op_attr_traits<attrId> {};

template <int attrId>
struct alltoallv_attr_traits : public common_op_attr_traits<attrId> {};

template <int attrId>
struct bcast_attr_traits : public common_op_attr_traits<attrId> {};

template <int attrId>
struct reduce_attr_traits : public common_op_attr_traits<attrId> {};

template <int attrId>
struct reduce_scatter_attr_traits : public common_op_attr_traits<attrId> {};

template <int attrId>
struct sparse_allreduce_attr_traits : public common_op_attr_traits<attrId> {};

template <int attrId>
struct barrier_attr_traits {};

/**
 * Traits specialization for allgatherv op attributes
 */
template <>
struct allgatherv_attr_traits<allgatherv_op_attr_id::vector_buf_id> {
    using type = int;
};

/**
 * Traits specialization for allreduce op attributes
 */
template <>
struct allreduce_attr_traits<allreduce_op_attr_id::reduction_fn_id> {
    using type = ccl_reduction_fn_t;
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
struct reduce_attr_traits<reduce_op_attr_id::reduction_fn_id> {
    using type = ccl_reduction_fn_t;
};

/**
 * Traits specialization for reduce_scatter op attributes
 */
template <>
struct reduce_scatter_attr_traits<reduce_scatter_op_attr_id::reduction_fn_id> {
    using type = ccl_reduction_fn_t;
};

/**
 * Traits specialization for sparse_allreduce op attributes
 */
template <>
struct sparse_allreduce_attr_traits<sparse_allreduce_op_attr_id::sparse_allreduce_completion_fn_id> {
    using type = ccl_sparse_allreduce_completion_fn_t;
};
template <>
struct sparse_allreduce_attr_traits<sparse_allreduce_op_attr_id::sparse_allreduce_alloc_fn_id> {
    using type = ccl_sparse_allreduce_alloc_fn_t;
};
template <>
struct sparse_allreduce_attr_traits<sparse_allreduce_op_attr_id::sparse_allreduce_fn_ctx_id> {
    using type = const void*;
};
template <>
struct sparse_allreduce_attr_traits<sparse_allreduce_op_attr_id::sparse_coalesce_mode_id> {
    using type = ccl_sparse_coalesce_mode_t;
};
}
