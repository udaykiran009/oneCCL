#pragma once

#include "common/comm/comm.hpp"
#include "common/datatype/datatype.hpp"

#define ICCL_INVALID_PROC_IDX (-1)

class iccl_sched;
class iccl_request;

enum iccl_coll_type
{
    iccl_coll_barrier =             0,
    iccl_coll_bcast =               1,
    iccl_coll_reduce =              2,
    iccl_coll_allreduce =           3,
    iccl_coll_allgatherv =          4,
    iccl_coll_sparse_allreduce =    5,
    iccl_coll_internal =            6,
    iccl_coll_none =                7
};

iccl_status_t iccl_coll_build_barrier(iccl_sched* sched);

iccl_status_t iccl_coll_build_bcast(iccl_sched* sched,
                                    void* buf,
                                    size_t count,
                                    iccl_datatype_internal_t dtype,
                                    size_t root);

iccl_status_t iccl_coll_build_reduce(iccl_sched* sched,
                                     const void* send_buf,
                                     void* recv_buf,
                                     size_t count,
                                     iccl_datatype_internal_t dtype,
                                     iccl_reduction_t reduction,
                                     size_t root);

iccl_status_t iccl_coll_build_allreduce(iccl_sched* sched,
                                        const void* send_buf,
                                        void* recv_buf,
                                        size_t count,
                                        iccl_datatype_internal_t dtype,
                                        iccl_reduction_t reduction);

iccl_status_t iccl_coll_build_allgatherv(iccl_sched* sched,
                                         const void* send_buf,
                                         void* recv_buf,
                                         size_t s_count,
                                         size_t* r_counts,
                                         iccl_datatype_internal_t dtype);

iccl_status_t iccl_coll_build_sparse_allreduce(iccl_sched* sched,
                                               const void* send_ind_buf,
                                               size_t send_ind_count,
                                               const void* send_val_buf,
                                               size_t send_val_count,
                                               void** recv_ind_buf,
                                               size_t* recv_ind_count,
                                               void** recv_val_buf,
                                               size_t* recv_val_count,
                                               iccl_datatype_internal_t index_dtype,
                                               iccl_datatype_internal_t value_dtype,
                                               iccl_reduction_t reduction);

const char* iccl_coll_type_to_str(iccl_coll_type type);

iccl_request* iccl_bcast_impl(void* buf,
                              size_t count,
                              iccl_datatype_t dtype,
                              size_t root,
                              const iccl_coll_attr_t* attributes,
                              iccl_comm* communicator);

iccl_request* iccl_reduce_impl(const void* send_buf,
                               void* recv_buf,
                               size_t count,
                               iccl_datatype_t dtype,
                               iccl_reduction_t reduction,
                               size_t root,
                               const iccl_coll_attr_t* attributes,
                               iccl_comm* communicator);

iccl_request* iccl_allreduce_impl(const void* send_buf,
                                  void* recv_buf,
                                  size_t count,
                                  iccl_datatype_t dtype,
                                  iccl_reduction_t reduction,
                                  const iccl_coll_attr_t* attributes,
                                  iccl_comm* communicator);

iccl_request* iccl_allgatherv_impl(const void* send_buf,
                                   size_t send_count,
                                   void* recv_buf,
                                   size_t* recv_counts,
                                   iccl_datatype_t dtype,
                                   const iccl_coll_attr_t* attributes,
                                   iccl_comm* communicator);

iccl_request* iccl_sparse_allreduce_impl(const void* send_ind_buf,
                                         size_t send_ind_count,
                                         const void* send_val_buf,
                                         size_t send_val_count,
                                         void** recv_ind_buf,
                                         size_t* recv_ind_count,
                                         void** recv_val_buf,
                                         size_t* recv_val_count,
                                         iccl_datatype_t index_dtype,
                                         iccl_datatype_t dtype,
                                         iccl_reduction_t reduction,
                                         const iccl_coll_attr_t* attributes,
                                         iccl_comm* communicator);

void iccl_barrier_impl(iccl_comm* communicator);
