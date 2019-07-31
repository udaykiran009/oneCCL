#include "coll/coll_algorithms.hpp"
#include "sched/entry_factory.hpp"

ccl_status_t ccl_coll_build_direct_barrier(ccl_sched *sched)
{
    LOG_DEBUG("build direct barrier");

    entry_factory::make_entry<barrier_entry>(sched);
    return ccl_status_success;
}

ccl_status_t ccl_coll_build_dissemination_barrier(ccl_sched *sched)
{
    LOG_DEBUG("build dissemination barrier");

    ccl_status_t status = ccl_status_success;
    int size, rank, src, dst, mask;
    size = sched->coll_param.comm->size();
    rank = sched->coll_param.comm->rank();

    if (size == 1)
        return status;

    mask = 0x1;
    while (mask < size) {
        dst = (rank + mask) % size;
        src = (rank - mask + size) % size;
        entry_factory::make_entry<send_entry>(sched, ccl_buffer(), 0, ccl_dtype_internal_char, dst);
        entry_factory::make_entry<recv_entry>(sched, ccl_buffer(), 0, ccl_dtype_internal_char, src);
        sched->add_barrier();
        mask <<= 1;
    }

    return status;
}
