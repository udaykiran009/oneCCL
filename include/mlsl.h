#pragma once

#include "mlsl_types.h"

#ifdef __cplusplus
extern "C" {
#endif

size_t MLSL_API mlsl_get_comm_rank(mlsl_comm_t comm);
size_t MLSL_API mlsl_get_comm_size(mlsl_comm_t comm);

mlsl_status_t MLSL_API mlsl_init();
mlsl_status_t MLSL_API mlsl_finalize();

mlsl_status_t MLSL_API mlsl_bcast(
    void* buf,
    size_t count,
    mlsl_datatype_t dtype,
    size_t root,
    const mlsl_coll_attr_t* attributes,
    mlsl_comm_t comm,
    mlsl_request_t* req);

mlsl_status_t MLSL_API mlsl_reduce(
    const void* send_buf,
    void* recv_buf,
    size_t count,
    mlsl_datatype_t dtype,
    mlsl_reduction_t reduction,
    size_t root,
    const mlsl_coll_attr_t* attributes,
    mlsl_comm_t comm,
    mlsl_request_t* req);

mlsl_status_t MLSL_API mlsl_allreduce(
    const void* send_buf,
    void* recv_buf,
    size_t count,
    mlsl_datatype_t dtype,
    mlsl_reduction_t reduction,
    const mlsl_coll_attr_t* attributes,
    mlsl_comm_t comm,
    mlsl_request_t* req);

mlsl_status_t MLSL_API mlsl_allgatherv(
    const void* send_buf,
    size_t send_count,
    void* recv_buf,
    size_t* recv_counts,
    mlsl_datatype_t dtype,
    const mlsl_coll_attr_t* attributes,
    mlsl_comm_t comm,
    mlsl_request_t* req);

mlsl_status_t MLSL_API mlsl_barrier(mlsl_comm_t comm);

mlsl_status_t MLSL_API mlsl_comm_create(mlsl_comm_t* comm, mlsl_comm_attr_t* comm_attr);

mlsl_status_t MLSL_API mlsl_comm_free(mlsl_comm_t comm);

mlsl_status_t MLSL_API mlsl_wait(mlsl_request_t req);

mlsl_status_t MLSL_API mlsl_test(mlsl_request_t req, int* is_completed);

#ifdef __cplusplus
}   /*extern C */
#endif
