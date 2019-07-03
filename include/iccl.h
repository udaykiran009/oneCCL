#pragma once

#include "iccl_types.h"

#ifdef __cplusplus
extern "C" {
#endif

iccl_status_t ICCL_API iccl_init();
iccl_status_t ICCL_API iccl_finalize();

iccl_status_t ICCL_API iccl_bcast(
    void* buf,
    size_t count,
    iccl_datatype_t dtype,
    size_t root,
    const iccl_coll_attr_t* attributes,
    iccl_comm_t communicator,
    iccl_request_t* req);

iccl_status_t ICCL_API iccl_reduce(
    const void* send_buf,
    void* recv_buf,
    size_t count,
    iccl_datatype_t dtype,
    iccl_reduction_t reduction,
    size_t root,
    const iccl_coll_attr_t* attributes,
    iccl_comm_t communicator,
    iccl_request_t* req);

iccl_status_t ICCL_API iccl_allreduce(
    const void* send_buf,
    void* recv_buf,
    size_t count,
    iccl_datatype_t dtype,
    iccl_reduction_t reduction,
    const iccl_coll_attr_t* attributes,
    iccl_comm_t communicator,
    iccl_request_t* req);

iccl_status_t ICCL_API iccl_allgatherv(
    const void* send_buf,
    size_t send_count,
    void* recv_buf,
    size_t* recv_counts,
    iccl_datatype_t dtype,
    const iccl_coll_attr_t* attributes,
    iccl_comm_t communicator,
    iccl_request_t* req);

iccl_status_t ICCL_API iccl_sparse_allreduce(
    const void* send_ind_buf,
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
    iccl_comm_t communicator,
    iccl_request_t* req);

iccl_status_t ICCL_API iccl_barrier(iccl_comm_t communicator);

iccl_status_t ICCL_API iccl_wait(iccl_request_t req);

iccl_status_t ICCL_API iccl_test(iccl_request_t req, int* is_completed);

iccl_status_t ICCL_API iccl_comm_create(iccl_comm_t* comm, iccl_comm_attr_t* comm_attr);

iccl_status_t ICCL_API iccl_comm_free(iccl_comm_t comm);

iccl_status_t ICCL_API iccl_get_comm_rank(iccl_comm_t comm, size_t* rank);

iccl_status_t ICCL_API iccl_get_comm_size(iccl_comm_t comm, size_t* size);

#ifdef __cplusplus
}   /*extern C */
#endif
