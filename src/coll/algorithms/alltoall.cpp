#include "coll/algorithms/algorithms.hpp"
#include "sched/entry/factory/entry_factory.hpp"

ccl_status_t ccl_coll_build_direct_alltoall(ccl_sched* sched,
                                            ccl_buffer send_buf,
                                            ccl_buffer recv_buf,
                                            size_t count,
                                            ccl_datatype_internal_t dtype,
                                            ccl_comm* comm)
{
    LOG_DEBUG("build direct alltoall");

    entry_factory::make_entry<alltoall_entry>(sched, send_buf, recv_buf,
                                              count, dtype, comm);
    return ccl_status_success;
}

//TODO: Will be used instead send\recv from parallelizer
#if 0
ccl_status_t ccl_coll_build_scatter_alltoall(ccl_sched* sched,
                                             ccl_buffer send_buf,
                                             ccl_buffer recv_buf,
                                             size_t count,
                                             ccl_datatype_internal_t dtype,
                                             ccl_comm* comm)
{
    LOG_DEBUG("build scatter alltoall");

    size_t comm_size     = comm->size();
    size_t this_rank     = comm->rank();
    size_t dtype_size    = ccl_datatype_get_size(dtype);
    size_t* offsets      = static_cast<size_t*>(CCL_MALLOC(comm_size * sizeof(size_t), "offsets"));
    ccl_status_t status = ccl_status_success;
    size_t bblock = comm_size;
    size_t ss, dst;
    offsets[0] = 0;
    for (size_t rank_idx = 1; rank_idx < comm_size; ++rank_idx)
    {
        offsets[rank_idx] = offsets[rank_idx - 1] + count * dtype_size;
    }

    if (send_buf != recv_buf)
    {
        // out-of-place case
        entry_factory::make_entry<copy_entry>(sched, send_buf, recv_buf + offsets[this_rank],
                                              count, dtype);
    }
    for (size_t idx = 0; idx < comm_size; idx += bblock) {
        ss = comm_size - idx < bblock ? comm_size - idx : bblock;
        /* do the communication -- post ss sends and receives: */
        for (size_t i = 0; i < ss; i++) {
            dst = (this_rank + i + idx) % comm_size;
            entry_factory::make_entry<recv_entry>(sched, recv_buf + offsets[dst],
                                                  count, dtype, dst);
        }

        for (size_t i = 0; i < ss; i++) {
            dst = (this_rank - i - idx + comm_size) % comm_size;
            entry_factory::make_entry<send_entry>(sched, send_buf + offsets[this_rank],
                                                  count, dtype, dst);
        }
    }


    CCL_FREE(offsets);
    return status;
}
#endif
