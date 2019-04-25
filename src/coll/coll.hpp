#pragma once

#include "common/comm/comm.hpp"
#include "common/datatype/datatype.hpp"

#define MLSL_INVALID_PROC_IDX (-1)

class mlsl_sched;
class mlsl_request;

enum mlsl_coll_type
{
    mlsl_coll_barrier =             0,
    mlsl_coll_bcast =               1,
    mlsl_coll_reduce =              2,
    mlsl_coll_allreduce =           3,
    mlsl_coll_allgatherv =          4,
    mlsl_coll_sparse_allreduce =    5,
    mlsl_coll_internal =            6,
    mlsl_coll_none =                7
};

mlsl_status_t mlsl_coll_build_barrier(mlsl_sched* sched);

mlsl_status_t mlsl_coll_build_bcast(mlsl_sched* sched,
                                    void* buf,
                                    size_t count,
                                    mlsl_datatype_internal_t dtype,
                                    size_t root);

mlsl_status_t mlsl_coll_build_reduce(mlsl_sched* sched,
                                     const void* send_buf,
                                     void* recv_buf,
                                     size_t count,
                                     mlsl_datatype_internal_t dtype,
                                     mlsl_reduction_t reduction,
                                     size_t root);

mlsl_status_t mlsl_coll_build_allreduce(mlsl_sched* sched,
                                        const void* send_buf,
                                        void* recv_buf,
                                        size_t count,
                                        mlsl_datatype_internal_t dtype,
                                        mlsl_reduction_t reduction);

mlsl_status_t mlsl_coll_build_sparse_allreduce(mlsl_sched* sched,
                                               const void* send_ind_buf,
                                               size_t send_ind_count,
                                               const void* send_val_buf,
                                               size_t send_val_count,
                                               void** recv_ind_buf,
                                               size_t* recv_ind_count,
                                               void** recv_val_buf,
                                               size_t* recv_val_count,
                                               mlsl_datatype_internal_t index_dtype,
                                               mlsl_datatype_internal_t value_dtype,
                                               mlsl_reduction_t reduction);

const char* mlsl_coll_type_to_str(mlsl_coll_type type);

mlsl_request* mlsl_bcast_impl(void* buf,
                              size_t count,
                              mlsl_datatype_t dtype,
                              size_t root,
                              const mlsl_coll_attr_t* attributes,
                              mlsl_comm* communicator);

mlsl_request* mlsl_reduce_impl(const void* send_buf,
                               void* recv_buf,
                               size_t count,
                               mlsl_datatype_t dtype,
                               mlsl_reduction_t reduction,
                               size_t root,
                               const mlsl_coll_attr_t* attributes,
                               mlsl_comm* communicator);

mlsl_request* mlsl_allreduce_impl(const void* send_buf,
                                  void* recv_buf,
                                  size_t count,
                                  mlsl_datatype_t dtype,
                                  mlsl_reduction_t reduction,
                                  const mlsl_coll_attr_t* attributes,
                                  mlsl_comm* communicator);

mlsl_request* mlsl_allgatherv_impl(const void* send_buf,
                                   size_t send_count,
                                   void* recv_buf,
                                   size_t* recv_counts,
                                   mlsl_datatype_t dtype,
                                   const mlsl_coll_attr_t* attributes,
                                   mlsl_comm* communicator);

mlsl_request* mlsl_sparse_allreduce_impl(const void* send_ind_buf,
                                         size_t send_ind_count,
                                         const void* send_val_buf,
                                         size_t send_val_count,
                                         void** recv_ind_buf,
                                         size_t* recv_ind_count,
                                         void** recv_val_buf,
                                         size_t* recv_val_count,
                                         mlsl_datatype_t index_dtype,
                                         mlsl_datatype_t dtype,
                                         mlsl_reduction_t reduction,
                                         const mlsl_coll_attr_t* attributes,
                                         mlsl_comm* communicator);

void mlsl_barrier_impl(mlsl_comm* communicator);
