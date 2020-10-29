/*
*
*  (C) 2001 by Argonne National Laboratory.
*      See COPYRIGHT in top-level directory.
*/

#include "coll/algorithms/algorithms.hpp"
#include "sched/entry/factory/entry_factory.hpp"

ccl::status ccl_coll_build_direct_barrier(ccl_sched* sched, ccl_comm* comm) {
    LOG_DEBUG("build direct barrier");

    entry_factory::make_entry<barrier_entry>(sched, comm);
    return ccl::status::success;
}

ccl::status ccl_coll_build_dissemination_barrier(ccl_sched* sched, ccl_comm* comm) {
    LOG_DEBUG("build dissemination barrier");

    ccl::status status = ccl::status::success;
    int size, rank, src, dst, mask;
    size = comm->size();
    rank = comm->rank();

    if (size == 1)
        return status;

    mask = 0x1;
    while (mask < size) {
        dst = (rank + mask) % size;
        src = (rank - mask + size) % size;
        entry_factory::make_entry<send_entry>(sched, ccl_buffer(), 0, ccl_datatype_int8, dst, comm);
        entry_factory::make_entry<recv_entry>(sched, ccl_buffer(), 0, ccl_datatype_int8, src, comm);
        sched->add_barrier();
        mask <<= 1;
    }

    return status;
}
