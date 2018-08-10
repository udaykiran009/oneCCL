#include "coll.h"
#include "sched.h"
#include "utils.h"

mlsl_status_t mlsl_coll_build_allreduce(mlsl_sched *sched, const void *send_buf, void *recv_buf,
                                        size_t count, mlsl_data_type_t dtype, mlsl_reduction_t op)
{
    return mlsl_status_success;
}

mlsl_status_t mlsl_allreduce(
    void *send_buf,
    void *recv_buf,
    size_t count,
    mlsl_data_type_t dtype,
    mlsl_reduction_t reduction,
    mlsl_request **req)
{
    mlsl_status_t status;
    mlsl_sched *sched;

    // TODO: add algorithm selection logic

    MLSL_CALL(mlsl_sched_create(&sched));
    MLSL_CALL(mlsl_coll_build_allreduce(sched, send_buf, recv_buf, count, dtype, reduction));
    MLSL_CALL(mlsl_sched_commit_with_type(sched, mlsl_sched_non_persistent));
    MLSL_CALL(mlsl_sched_start(sched, req));

    return status;
}
