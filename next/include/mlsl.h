#ifndef MLSL_H
#define MLSL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "mlsl_types.h"

size_t MLSL_API mlsl_get_proc_idx();
size_t MLSL_API mlsl_get_proc_count();
size_t MLSL_API mlsl_get_dtype_size(mlsl_data_type_t dtype);

mlsl_status_t MLSL_API mlsl_init();
mlsl_status_t MLSL_API mlsl_finalize();

/* regular collectives API */

mlsl_status_t MLSL_API mlsl_allreduce(
    void *send_buf,
    void *recv_buf,
    size_t count,
    mlsl_data_type_t dtype,
    mlsl_reduction_t reduction,
    mlsl_request_t *req);

mlsl_status_t MLSL_API mlsl_wait(mlsl_request_t req);

#ifdef __cplusplus
}
#endif

#endif /* MLSL_H */
