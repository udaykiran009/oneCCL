#pragma once

#include "atl/atl.h"

typedef struct {
    int wait_dst;

    size_t src_peer;
    size_t dst_peer;

    volatile uint64_t sync_flag; // src side will write here the index of iteration it completed
    atl_mr_t* sync_flag_mr;

    uint64_t* sync_flags; // pre-computed values (iteration indexes) to be written to dst side
    atl_mr_t* sync_flags_mr;

    volatile uint64_t
        dst_ready_flag; // dst side will write here '1' to indicate it is ready for write ops
    atl_mr_t* dst_ready_flag_mr;

    uint64_t dst_ready_value;
    atl_mr_t* dst_ready_value_mr;

    atl_mr_t remote_sync_flag_mr;
    atl_mr_t remote_dst_ready_flag_mr;

    atl_mr_t* send_buf_mr;
    atl_mr_t* recv_buf_mr;
    atl_mr_t* tmp_buf_mr;

    atl_mr_t remote_rs_dst_buf_mr;
    atl_mr_t remote_recv_buf_mr;

} ccl_rma_ring_allreduce_handler;
