#pragma once

#include "ccl_types.h"

#ifdef __cplusplus
extern "C" {
#endif

ccl_status_t CCL_API ccl_init();
ccl_status_t CCL_API ccl_finalize();

ccl_status_t CCL_API ccl_bcast(
    void* buf,
    size_t count,
    ccl_datatype_t dtype,
    size_t root,
    const ccl_coll_attr_t* attributes,
    ccl_comm_t communicator,
    ccl_request_t* req);

ccl_status_t CCL_API ccl_reduce(
    const void* send_buf,
    void* recv_buf,
    size_t count,
    ccl_datatype_t dtype,
    ccl_reduction_t reduction,
    size_t root,
    const ccl_coll_attr_t* attributes,
    ccl_comm_t communicator,
    ccl_request_t* req);

ccl_status_t CCL_API ccl_allreduce(
    const void* send_buf,
    void* recv_buf,
    size_t count,
    ccl_datatype_t dtype,
    ccl_reduction_t reduction,
    const ccl_coll_attr_t* attributes,
    ccl_comm_t communicator,
    ccl_request_t* req);

ccl_status_t CCL_API ccl_allgatherv(
    const void* send_buf,
    size_t send_count,
    void* recv_buf,
    size_t* recv_counts,
    ccl_datatype_t dtype,
    const ccl_coll_attr_t* attributes,
    ccl_comm_t communicator,
    ccl_request_t* req);

ccl_status_t CCL_API ccl_sparse_allreduce(
    const void* send_ind_buf,
    size_t send_ind_count,
    const void* send_val_buf,
    size_t send_val_count,
    void** recv_ind_buf,
    size_t* recv_ind_count,
    void** recv_val_buf,
    size_t* recv_val_count,
    ccl_datatype_t index_dtype,
    ccl_datatype_t dtype,
    ccl_reduction_t reduction,
    const ccl_coll_attr_t* attributes,
    ccl_comm_t communicator,
    ccl_request_t* req);

ccl_status_t CCL_API ccl_barrier(ccl_comm_t communicator);

ccl_status_t CCL_API ccl_wait(ccl_request_t req);

ccl_status_t CCL_API ccl_test(ccl_request_t req, int* is_completed);

ccl_status_t CCL_API ccl_comm_create(ccl_comm_t* comm, ccl_comm_attr_t* comm_attr);

ccl_status_t CCL_API ccl_comm_free(ccl_comm_t comm);

ccl_status_t CCL_API ccl_get_comm_rank(ccl_comm_t comm, size_t* rank);

ccl_status_t CCL_API ccl_get_comm_size(ccl_comm_t comm, size_t* size);

#ifdef __cplusplus
}   /*extern C */
#endif
