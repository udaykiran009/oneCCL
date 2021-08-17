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
    ccl_coll_algo hint_algo;
};
