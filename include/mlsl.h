#ifndef MLSL_H
#define MLSL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "mlsl_types.h"

size_t MLSL_API mlsl_get_proc_idx();
size_t MLSL_API mlsl_get_proc_count();

mlsl_status_t MLSL_API mlsl_init();
mlsl_status_t MLSL_API mlsl_finalize();

mlsl_status_t MLSL_API mlsl_bcast(
    void *buf,
    size_t count,
    mlsl_data_type_t dtype,
    size_t root,
    const mlsl_coll_attr_t *attributes,
    mlsl_request_t *req);

mlsl_status_t MLSL_API mlsl_reduce(
    const void *send_buf,
    void *recv_buf,
    size_t count,
    mlsl_data_type_t dtype,
    mlsl_reduction_t reduction,
    size_t root,
    const mlsl_coll_attr_t *attributes,
    mlsl_request_t *req);

mlsl_status_t MLSL_API mlsl_allreduce(
    const void *send_buf,
    void *recv_buf,
    size_t count,
    mlsl_data_type_t dtype,
    mlsl_reduction_t reduction,
    const mlsl_coll_attr_t *attributes,
    mlsl_request_t *req);

mlsl_status_t MLSL_API mlsl_allgatherv(
    const void *send_buf,
    size_t send_count,
    void *recv_buf,
    size_t *recv_counts,
    mlsl_data_type_t dtype,
    const mlsl_coll_attr_t *attributes,
    mlsl_request_t *req);

mlsl_status_t MLSL_API mlsl_barrier();

// TODO: add comm create/free

mlsl_status_t MLSL_API mlsl_wait(mlsl_request_t req);
mlsl_status_t MLSL_API mlsl_test(mlsl_request_t req, int *is_completed);

#ifdef __cplusplus
}
#endif

#endif /* MLSL_H */
