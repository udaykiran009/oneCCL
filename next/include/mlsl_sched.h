#ifndef MLSL_SCHED_H
#define MLSL_SCHED_H

#ifdef __cplusplus
extern "C" {
#endif

#include "mlsl_types.h"

/* persistent collectives API */

/* workflow:
 * 1. create sched (empty or pre-filled)
 * 2. adjust sched (by adding primitives or by changing phases like prolog/epilog/reduce)
 * 3. commit sched
 * 4. start & wait sched multiple times
 * 5. free sched
 */

mlsl_status_t MLSL_API mlsl_sched_create(mlsl_sched_t *sched);
mlsl_status_t MLSL_API mlsl_sched_commit(mlsl_sched_t sched);
mlsl_status_t MLSL_API mlsl_sched_start(mlsl_sched_t sched, mlsl_request_t *req);
mlsl_status_t MLSL_API mlsl_sched_free(mlsl_sched_t sched);

/* schedule adjustment functions */

mlsl_status_t MLSL_API mlsl_sched_add_send(
    mlsl_sched_t sched, const void *buf, size_t count,
    mlsl_data_type_t dtype, size_t dest);

mlsl_status_t MLSL_API mlsl_sched_add_recv(
    mlsl_sched_t sched, void *buf, size_t count,
    mlsl_data_type_t data_type, size_t src);

mlsl_status_t MLSL_API mlsl_sched_add_reduce(
    mlsl_sched_t sched, const void *in_buf,
    void *inout_buf, size_t count,
    mlsl_data_type_t dtype, mlsl_reduction_t reduction);

mlsl_status_t MLSL_API mlsl_sched_add_copy(
    mlsl_sched_t sched, const void *in_buf,
    void *out_buf, size_t count, mlsl_data_type_t dtype);

mlsl_status_t MLSL_API mlsl_sched_add_barrier(mlsl_sched_t sched);

mlsl_status_t MLSL_API mlsl_sched_set_prologue(mlsl_sched_t sched, mlsl_sched_prolog_fn_t fn);
mlsl_status_t MLSL_API mlsl_sched_set_epilogue(mlsl_sched_t sched, mlsl_sched_epilog_fn_t fn);
mlsl_status_t MLSL_API mlsl_sched_set_reduction(mlsl_sched_t sched, mlsl_sched_reduction_fn_t fn);

/* pre-filled schedules */

mlsl_status_t MLSL_API mlsl_sched_allreduce(
    void *send_buf,
    void *recv_buf,
    size_t count,
    mlsl_data_type_t dtype,
    mlsl_reduction_t reduction,
    mlsl_sched_t **sched);

#ifdef __cplusplus
}
#endif

#endif /* MLSL_SCHED_H */
