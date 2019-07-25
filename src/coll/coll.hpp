#pragma once

#include "common/comm/comm.hpp"
#include "common/stream/stream.hpp"
#include "common/datatype/datatype.hpp"
#include "common/utils/buffer.hpp"

#define CCL_INVALID_PROC_IDX (-1)

class ccl_sched;
class ccl_request;

enum ccl_coll_type
{
    ccl_coll_barrier =             0,
    ccl_coll_bcast =               1,
    ccl_coll_reduce =              2,
    ccl_coll_allreduce =           3,
    ccl_coll_allgatherv =          4,
    ccl_coll_sparse_allreduce =    5,
    ccl_coll_internal =            6,
    ccl_coll_none =                7
};

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

ccl_status_t ccl_coll_build_allreduce(ccl_sched* sched,
                                      ccl_buffer send_buf,
                                      ccl_buffer recv_buf,
                                      size_t count,
                                      ccl_datatype_internal_t dtype,
                                      ccl_reduction_t reduction);

ccl_status_t ccl_coll_build_allgatherv(ccl_sched* sched,
                                       ccl_buffer send_buf,
                                       size_t s_count,
                                       ccl_buffer recv_buf,
                                       size_t* r_counts,
                                       ccl_datatype_internal_t dtype);

ccl_status_t ccl_coll_build_sparse_allreduce(ccl_sched* sched,
                                             ccl_buffer send_ind_buf,
                                             size_t send_ind_count,
                                             ccl_buffer send_val_buf,
                                             size_t send_val_count,
                                             ccl_buffer recv_ind_buf,
                                             size_t* recv_ind_count,
                                             ccl_buffer recv_val_buf,
                                             size_t* recv_val_count,
                                             ccl_datatype_internal_t index_dtype,
                                             ccl_datatype_internal_t value_dtype,
                                             ccl_reduction_t reduction);

const char* ccl_coll_type_to_str(ccl_coll_type type);

ccl_request* ccl_bcast_impl(void* buf,
                            size_t count,
                            ccl_datatype_t dtype,
                            size_t root,
                            const ccl_coll_attr_t* attributes,
                            ccl_comm* communicator,
                            const ccl_stream* stream);

ccl_request* ccl_reduce_impl(const void* send_buf,
                             void* recv_buf,
                             size_t count,
                             ccl_datatype_t dtype,
                             ccl_reduction_t reduction,
                             size_t root,
                             const ccl_coll_attr_t* attributes,
                             ccl_comm* communicator,
                             const ccl_stream* stream);

ccl_request* ccl_allreduce_impl(const void* send_buf,
                                void* recv_buf,
                                size_t count,
                                ccl_datatype_t dtype,
                                ccl_reduction_t reduction,
                                const ccl_coll_attr_t* attributes,
                                ccl_comm* communicator,
                                const ccl_stream* stream);

ccl_request* ccl_allgatherv_impl(const void* send_buf,
                                 size_t send_count,
                                 void* recv_buf,
                                 size_t* recv_counts,
                                 ccl_datatype_t dtype,
                                 const ccl_coll_attr_t* attributes,
                                 ccl_comm* communicator,
                                 const ccl_stream* stream);

ccl_request* ccl_sparse_allreduce_impl(const void* send_ind_buf, size_t send_ind_count,
                                       const void* send_val_buf, size_t send_val_count,
                                       void** recv_ind_buf, size_t* recv_ind_count,
                                       void** recv_val_buf, size_t* recv_val_count,
                                       ccl_datatype_t index_dtype, ccl_datatype_t dtype,
                                       ccl_reduction_t reduction, const ccl_coll_attr_t* attributes,
                                       ccl_comm* communicator, const ccl_stream* stream);

void ccl_barrier_impl(ccl_comm* communicator, const ccl_stream* stream);
