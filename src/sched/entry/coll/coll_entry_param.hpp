#pragma once

#include "coll/coll.hpp"

struct ccl_coll_entry_param {
    ccl_coll_type ctype;
    ccl_buffer send_buf;
    ccl_buffer recv_buf;
    size_t count;
    size_t send_count;
    const size_t* send_counts;
    const size_t* recv_counts;
    ccl_datatype dtype;
    ccl::reduction reduction;
    int root;
    ccl_comm* comm;
    ccl_stream* stream;

    ccl_coll_param to_coll_param() const {
        ccl_coll_param param;
        param.ctype = ctype;
        param.send_buf = send_buf.get_ptr();
        param.recv_buf = recv_buf.get_ptr();
        param.count = count;
        param.send_count = send_count;
        param.send_counts = send_counts;
        param.recv_counts = recv_counts;
        param.dtype = dtype;
        param.reduction = reduction;
        param.root = root;
        param.comm = comm;
        param.stream = stream;
        return param;
    }
};
