#include "coll/coll_algorithms.hpp"
#include "sched/entry_factory.hpp"

mlsl_status_t mlsl_coll_build_direct_barrier(mlsl_sched *sched)
{
    LOG_DEBUG("build direct barrier");

    entry_factory::make_barrier_entry(sched);
    return mlsl_status_success;
}

mlsl_status_t mlsl_coll_build_dissemination_barrier(mlsl_sched *sched)
{
    LOG_DEBUG("build dissemination barrier");

    mlsl_status_t status = mlsl_status_success;
    int size, rank, src, dst, mask;
    size = sched->coll_param.comm->size();
    rank = sched->coll_param.comm->rank();

    if (size == 1)
        return status;

    mask = 0x1;
    while (mask < size) {
        dst = (rank + mask) % size;
        src = (rank - mask + size) % size;
        entry_factory::make_send_entry(sched, NULL, 0, mlsl_dtype_internal_char, dst);
        entry_factory::make_recv_entry(sched, NULL, 0, mlsl_dtype_internal_char, src);
        sched->add_barrier();
        mask <<= 1;
    }

    return status;
}
