#pragma once

#ifndef CCL_PRODUCT_FULL
#error "Do not include this file directly. Please include 'ccl.hpp'"
#endif

namespace ccl {

/**
 * Common operation attributes id
 */
enum class common_op_attr_id : int {
    version,
    prolog_fn,
    epilog_fn,
    priority,
    synchronous,
    to_cache,
    match_id,

    last_value
};

/**
 * Collective attributes
 */
enum class allgatherv_op_attr_id : int {
    op_id_offset = static_cast<typename std::underlying_type<common_op_attr_id>::type>(common_op_attr_id::last_value),
    vector_buf = op_id_offset,

    last_value
};

enum class allreduce_op_attr_id : int {
    op_id_offset = static_cast<typename std::underlying_type<common_op_attr_id>::type>(common_op_attr_id::last_value),
    reduction_fn = op_id_offset,

    last_value
};

enum class alltoall_op_attr_id : int {
    op_id_offset = static_cast<typename std::underlying_type<common_op_attr_id>::type>(common_op_attr_id::last_value),

    last_value
};

enum class alltoallv_op_attr_id : int {
    op_id_offset = static_cast<typename std::underlying_type<common_op_attr_id>::type>(common_op_attr_id::last_value),

    last_value
};

enum class bcast_op_attr_id : int {
    op_id_offset = static_cast<typename std::underlying_type<common_op_attr_id>::type>(common_op_attr_id::last_value),

    last_value
};

enum class reduce_op_attr_id : int {
    op_id_offset = static_cast<typename std::underlying_type<common_op_attr_id>::type>(common_op_attr_id::last_value),

    reduction_fn = op_id_offset,
    last_value
};

enum class reduce_scatter_op_attr_id : int {
    op_id_offset = static_cast<typename std::underlying_type<common_op_attr_id>::type>(common_op_attr_id::last_value),

    reduction_fn = op_id_offset,
    last_value
};

enum class sparse_allreduce_op_attr_id : int {
    op_id_offset = static_cast<typename std::underlying_type<common_op_attr_id>::type>(common_op_attr_id::last_value),

    sparse_allreduce_completion_fn = op_id_offset,
    sparse_allreduce_alloc_fn,
    sparse_allreduce_fn_ctx,
    sparse_coalesce_mode,
    last_value
};

enum class barrier_op_attr_id : int {
    version,

    last_value
};
}
