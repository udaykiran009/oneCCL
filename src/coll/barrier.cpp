#include "coll/coll_algorithms.hpp"
#include "sched/entry_factory.hpp"

iccl_status_t iccl_coll_build_direct_barrier(iccl_sched *sched)
{
    LOG_DEBUG("build direct barrier");

    entry_factory::make_barrier_entry(sched);
    return iccl_status_success;
}

iccl_status_t iccl_coll_build_dissemination_barrier(iccl_sched *sched)
{
    LOG_DEBUG("build dissemination barrier");

    iccl_status_t status = iccl_status_success;
    int size, rank, src, dst, mask;
    size = sched->coll_param.comm->size();
    rank = sched->coll_param.comm->rank();

    if (size == 1)
        return status;

    mask = 0x1;
    while (mask < size) {
        dst = (rank + mask) % size;
        src = (rank - mask + size) % size;
        entry_factory::make_send_entry(sched, iccl_buffer(), 0, iccl_dtype_internal_char, dst);
        entry_factory::make_recv_entry(sched, iccl_buffer(), 0, iccl_dtype_internal_char, src);
        sched->add_barrier();
        mask <<= 1;
    }

    return status;
}
