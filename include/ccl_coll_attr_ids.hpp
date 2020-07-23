#pragma once

#ifndef CCL_PRODUCT_FULL
#error "Do not include this file directly. Please include 'ccl.hpp'"
#endif

namespace ccl {

/**
 * Common operation attributes id
 */
struct common_op_attr_id {
    enum : int {
        version_id,
        prolog_fn_id,
        epilog_fn_id,
        priority_id,
        synchronous_id,
        to_cache_id,
        match_id,

        last_value
    };
    common_op_attr_id() = delete;
};

/**
 * Collective attributes
 */
struct allgatherv_op_attr_id : common_op_attr_id {
    enum : int {
        op_id_offset = common_op_attr_id::last_value,
        vector_buf_id = op_id_offset,

        last_value
    };
    allgatherv_op_attr_id() = delete;
};

struct allreduce_op_attr_id : common_op_attr_id {
    enum : int {
        op_id_offset = common_op_attr_id::last_value,
        reduction_fn_id = op_id_offset,

        last_value
    };
    allreduce_op_attr_id() = delete;
};

struct alltoall_op_attr_id : common_op_attr_id {
    enum : int {
        op_id_offset = common_op_attr_id::last_value,

        last_value
    };
    alltoall_op_attr_id() = delete;
};

struct alltoallv_op_attr_id : common_op_attr_id {
    enum : int {
        op_id_offset = common_op_attr_id::last_value,

        last_value
    };
    alltoallv_op_attr_id() = delete;
};

struct bcast_op_attr_id : common_op_attr_id {
    enum : int {
        op_id_offset = common_op_attr_id::last_value,

        last_value
    };
    bcast_op_attr_id() = delete;
};

struct reduce_op_attr_id : common_op_attr_id {
    enum : int {
        op_id_offset = common_op_attr_id::last_value,

        reduction_fn_id = op_id_offset,
        last_value
    };
    reduce_op_attr_id() = delete;
};

struct reduce_scatter_op_attr_id : common_op_attr_id {
    enum : int {
        op_id_offset = common_op_attr_id::last_value,

        reduction_fn_id = op_id_offset,
        last_value
    };
    reduce_scatter_op_attr_id() = delete;
};

struct sparse_allreduce_op_attr_id : common_op_attr_id {
    enum : int {
        op_id_offset = common_op_attr_id::last_value,

        sparse_allreduce_completion_fn_id = op_id_offset,
        sparse_allreduce_alloc_fn_id,
        sparse_allreduce_fn_ctx_id,
        sparse_coalesce_mode_id,
        last_value
    };
    sparse_allreduce_op_attr_id() = delete;
};

struct barrier_op_attr_id {
    enum : int {
        last_value
    };
    barrier_op_attr_id() = delete;
};
}
