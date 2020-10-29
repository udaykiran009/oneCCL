#pragma once

#include "coll/algorithms/algorithms_enum.hpp"
#include "common/datatype/datatype.hpp"

#include "oneapi/ccl/ccl_type_traits.hpp"
#include "oneapi/ccl/ccl_stream_attr_ids.hpp"
#include "oneapi/ccl/ccl_stream_attr_ids_traits.hpp"
#include "oneapi/ccl/ccl_stream.hpp"
#include "oneapi/ccl/ccl_coll_attr_ids.hpp"
#include "oneapi/ccl/ccl_coll_attr_ids_traits.hpp"
#include "oneapi/ccl/ccl_coll_attr.hpp"

class ccl_comm;

#ifdef CCL_ENABLE_SYCL
#include <CL/sycl.hpp>
typedef cl::sycl::buffer<int8_t, 1> ccl_sycl_buffer_t;

template <class native_type>
using ccl_sycl_typed_buffer_t = cl::sycl::buffer<native_type, 1>;

/* ordering should be aligned with ccl::datatype */
using ccl_sycle_buffer_one_dim_types = std::tuple<ccl_sycl_typed_buffer_t<int8_t>,
                                                  ccl_sycl_typed_buffer_t<uint8_t>,
                                                  ccl_sycl_typed_buffer_t<int16_t>,
                                                  ccl_sycl_typed_buffer_t<uint16_t>,
                                                  ccl_sycl_typed_buffer_t<int32_t>,
                                                  ccl_sycl_typed_buffer_t<uint32_t>,
                                                  ccl_sycl_typed_buffer_t<int64_t>,
                                                  ccl_sycl_typed_buffer_t<uint64_t>,
                                                  ccl_sycl_typed_buffer_t<float>, //unsupported
                                                  ccl_sycl_typed_buffer_t<float>,
                                                  ccl_sycl_typed_buffer_t<double>,
                                                  ccl_sycl_typed_buffer_t<float>>; //unsupported
#endif /* CCL_ENABLE_SYCL */

#define CCL_INVALID_PROC_IDX (-1)

struct ccl_coll_attr {
    ccl_coll_attr() = default;
    ccl_coll_attr(const ccl_coll_attr&) = default;
    ccl_coll_attr& operator=(const ccl_coll_attr&) = default;

    //TODO temporary solution for type convertation, ccl_coll_attr would be depreacated
    ccl_coll_attr(const ccl::allgatherv_attr& attr);
    ccl_coll_attr(const ccl::allreduce_attr& attr);
    ccl_coll_attr(const ccl::alltoall_attr& attr);
    ccl_coll_attr(const ccl::alltoallv_attr& attr);
    ccl_coll_attr(const ccl::barrier_attr& attr);
    ccl_coll_attr(const ccl::broadcast_attr& attr);
    ccl_coll_attr(const ccl::reduce_attr& attr);
    ccl_coll_attr(const ccl::reduce_scatter_attr& attr);
    ccl_coll_attr(const ccl::sparse_allreduce_attr& attr);

    ccl_coll_attr(ccl_coll_attr&&) = default;
    ccl_coll_attr& operator=(ccl_coll_attr&&) = default;

    ccl::prologue_fn prologue_fn = nullptr;
    ccl::epilogue_fn epilogue_fn = nullptr;
    ccl::reduction_fn reduction_fn = nullptr;

    size_t priority = 0;
    int synchronous = 0;
    int to_cache = 0;
    int vector_buf = 0;
    std::string match_id{};

    ccl::sparse_allreduce_completion_fn sparse_allreduce_completion_fn = nullptr;
    ccl::sparse_allreduce_alloc_fn sparse_allreduce_alloc_fn = nullptr;
    const void* sparse_allreduce_fn_ctx = nullptr;
    ccl::sparse_coalesce_mode sparse_coalesce_mode = ccl::sparse_coalesce_mode::regular;
};

struct ccl_coll_sparse_param {
    const void* send_ind_buf;
    size_t send_ind_count;
    const void* send_val_buf;
    size_t send_val_count;
    void* recv_ind_buf;
    size_t recv_ind_count;
    void* recv_val_buf;
    size_t recv_val_count;
    ccl_datatype itype;
};

struct ccl_coll_param {
    ccl_coll_type ctype;
    void* buf;
    const void* send_buf;
    void* recv_buf;
    size_t count;
    size_t send_count;
    const size_t* send_counts;
    const size_t* recv_counts;
    ccl_datatype dtype;
    ccl::reduction reduction;
    size_t root;
    const ccl_stream* stream;
    ccl_comm* comm;
    ccl_coll_sparse_param sparse_param;

#ifdef CCL_ENABLE_SYCL
    ccl_sycl_buffer_t* sycl_send_buf;
    ccl_sycl_buffer_t* sycl_recv_buf;
    ccl_sycl_buffer_t* sycl_buf;
#endif /* CCL_ENABLE_SYCL */
};

/*
    explicitly split coll_param and coll_param_copy
    to separate coll_param structure which is used for interaction between different modules
    and coll_param_copy which is used as storage for user options
*/
struct ccl_coll_param_copy {
    /* keep copy of user options which can be invalidated after collective call */

    std::vector<void*> ag_recv_bufs;
    std::vector<size_t> ag_recv_counts;

    std::vector<size_t> a2av_send_counts;
    std::vector<size_t> a2av_recv_counts;
};