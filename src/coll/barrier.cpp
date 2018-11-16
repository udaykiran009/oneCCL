#include "coll/coll_algorithms.hpp"

mlsl_status_t mlsl_coll_build_dissemination_barrier(mlsl_sched *sched)
{
    MLSL_LOG(DEBUG, "build dissemination barrier");

    mlsl_status_t status = mlsl_status_success;
    int size, rank, src, dst, mask;
    size = sched->coll_param.comm->size;
    rank = sched->coll_param.comm->rank;

    if (size == 1)
        return status;

    mask = 0x1;
    while (mask < size) {
        dst = (rank + mask) % size;
        src = (rank - mask + size) % size;
        MLSL_CALL(mlsl_sched_add_send(sched, NULL, 0, mlsl_dtype_char, dst));
        MLSL_CALL(mlsl_sched_add_recv(sched, NULL, 0, mlsl_dtype_char, src));
        MLSL_CALL(mlsl_sched_add_barrier(sched));
        mask <<= 1;
    }

    return status;
}
