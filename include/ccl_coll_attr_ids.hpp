#pragma once

#ifndef CCL_PRODUCT_FULL
#error "Do not include this file directly. Please include 'ccl.hpp'"
#endif

namespace ccl {

/**
 * Common operation attributes id
 */
enum class common_op_attr_id : int {
    version_id,
    prolog_fn_id,
    epilog_fn_id,
    priority_id,
    synchronous_id,
    to_cache_id,
    match_id,

    last_value
};

/**
 * Collective attributes
 */
enum class allgatherv_op_attr_id : int {
    op_id_offset = static_cast<typename std::underlying_type<common_op_attr_id>::type>(last_value),
    vector_buf_id = op_id_offset,

    last_value
};

enum class allreduce_op_attr_id : int {
    op_id_offset = static_cast<typename std::underlying_type<common_op_attr_id>::type>(last_value),
    reduction_fn_id = op_id_offset,

    last_value
};

enum class alltoall_op_attr_id : int {
    op_id_offset = static_cast<typename std::underlying_type<common_op_attr_id>::type>(last_value),

    last_value
};

enum class alltoallv_op_attr_id : int {
    op_id_offset = static_cast<typename std::underlying_type<common_op_attr_id>::type>(last_value),

    last_value
};

enum class bcast_op_attr_id : int {
    op_id_offset = static_cast<typename std::underlying_type<common_op_attr_id>::type>(last_value),

    last_value
};

enum class reduce_op_attr_id : int {
    op_id_offset = static_cast<typename std::underlying_type<common_op_attr_id>::type>(last_value),

    reduction_fn_id = op_id_offset,
    last_value
};

enum class reduce_scatter_op_attr_id : int {
    op_id_offset = static_cast<typename std::underlying_type<common_op_attr_id>::type>(last_value),

    reduction_fn_id = op_id_offset,
    last_value
};

enum class sparse_allreduce_op_attr_id : int {
    op_id_offset = static_cast<typename std::underlying_type<common_op_attr_id>::type>(last_value),

    sparse_allreduce_completion_fn_id = op_id_offset,
    sparse_allreduce_alloc_fn_id,
    sparse_allreduce_fn_ctx_id,
    sparse_coalesce_mode_id,
    last_value
};

enum class barrier_op_attr_id : int {

    last_value
};
}
