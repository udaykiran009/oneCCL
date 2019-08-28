#pragma once

#include "common/comm/comm.hpp"
#include "common/stream/stream.hpp"
#include "common/datatype/datatype.hpp"
#include "common/utils/buffer.hpp"
#include "common/global/global.hpp"

#ifdef ENABLE_SYCL
#include <CL/sycl.hpp>
typedef cl::sycl::buffer<char, 1> ccl_sycl_buffer_t;
#endif /* ENABLE_SYCL */

#define CCL_INVALID_PROC_IDX (-1)

class ccl_sched;
class ccl_request;


struct ccl_coll_attr
{
    ccl_prologue_fn_t prologue_fn;
    ccl_epilogue_fn_t epilogue_fn;
    ccl_reduction_fn_t reduction_fn;
    size_t priority;
    int synchronous;
    int to_cache;
    std::string match_id;
};

struct ccl_coll_sparse_param
{
    const void* send_ind_buf;
    size_t send_val_count;
    const void* send_val_buf;
    size_t send_ind_count;
    void** recv_ind_buf;
    size_t* recv_ind_count;
    void** recv_val_buf;
    size_t* recv_val_count;
    ccl_datatype_internal_t itype;
};

struct ccl_coll_param
{
    ccl_coll_type ctype;
    void* buf;
    const void* send_buf;
    void* recv_buf;
    size_t count;
    size_t send_count;
    const size_t* recv_counts;
    ccl_datatype_internal_t dtype;
    ccl_reduction_t reduction;
    size_t root;
    const ccl_stream* stream;
    ccl_comm* comm;
    ccl_coll_sparse_param sparse_param;

#ifdef ENABLE_SYCL
    ccl_sycl_buffer_t* sycl_send_buf;
    ccl_sycl_buffer_t* sycl_recv_buf;
    ccl_sycl_buffer_t* sycl_buf;
#endif /* ENABLE_SYCL */

};

const char* ccl_coll_type_to_str(ccl_coll_type type);

ccl_status_t ccl_coll_build_allgatherv(ccl_sched* sched,
                                       ccl_buffer send_buf,
                                       size_t send_count,
                                       ccl_buffer recv_buf,
                                       const size_t* recv_counts,
                                       ccl_datatype_internal_t dtype);

ccl_status_t ccl_coll_build_allreduce(ccl_sched* sched,
                                      ccl_buffer send_buf,
                                      ccl_buffer recv_buf,
                                      size_t count,
                                      ccl_datatype_internal_t dtype,
                                      ccl_reduction_t reduction);

ccl_status_t ccl_coll_build_barrier(ccl_sched* sched);

ccl_status_t ccl_coll_build_bcast(ccl_sched* sched,
                                  ccl_buffer buf,
                                  size_t count,
                                  ccl_datatype_internal_t dtype,
                                  size_t root);

ccl_status_t ccl_coll_build_reduce(ccl_sched* sched,
                                   ccl_buffer send_buf,
                                   ccl_buffer recv_buf,
                                   size_t count,
                                   ccl_datatype_internal_t dtype,
                                   ccl_reduction_t reduction,
                                   size_t root);

ccl_status_t ccl_coll_build_sparse_allreduce(ccl_sched* sched,
                                             ccl_buffer send_ind_buf, size_t send_ind_count,
                                             ccl_buffer send_val_buf, size_t send_val_count,
                                             ccl_buffer recv_ind_buf, size_t* recv_ind_count,
                                             ccl_buffer recv_val_buf, size_t* recv_val_count,
                                             ccl_datatype_internal_t index_dtype,
                                             ccl_datatype_internal_t value_dtype,
                                             ccl_reduction_t reduction);

ccl_request* ccl_allgatherv_impl(const void* send_buf,
                                 size_t send_count,
                                 void* recv_buf,
                                 const size_t* recv_counts,
                                 ccl_datatype_t dtype,
                                 const ccl_coll_attr_t* attr,
                                 ccl_comm* comm,
                                 const ccl_stream* stream);

ccl_request* ccl_allreduce_impl(const void* send_buf,
                                void* recv_buf,
                                size_t count,
                                ccl_datatype_t dtype,
                                ccl_reduction_t reduction,
                                const ccl_coll_attr_t* attr,
                                ccl_comm* comm,
                                const ccl_stream* stream);

void ccl_barrier_impl(ccl_comm* comm,
                      const ccl_stream* stream);

ccl_request* ccl_bcast_impl(void* buf,
                            size_t count,
                            ccl_datatype_t dtype,
                            size_t root,
                            const ccl_coll_attr_t* attr,
                            ccl_comm* comm,
                            const ccl_stream* stream);

ccl_request* ccl_reduce_impl(const void* send_buf,
                             void* recv_buf,
                             size_t count,
                             ccl_datatype_t dtype,
                             ccl_reduction_t reduction,
                             size_t root,
                             const ccl_coll_attr_t* attr,
                             ccl_comm* comm,
                             const ccl_stream* stream);

ccl_request* ccl_sparse_allreduce_impl(const void* send_ind_buf, size_t send_ind_count,
                                       const void* send_val_buf, size_t send_val_count,
                                       void** recv_ind_buf, size_t* recv_ind_count,
                                       void** recv_val_buf, size_t* recv_val_count,
                                       ccl_datatype_t index_dtype, ccl_datatype_t dtype,
                                       ccl_reduction_t reduction, const ccl_coll_attr_t* attr,
                                       ccl_comm* comm, const ccl_stream* stream);
