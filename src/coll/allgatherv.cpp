#include "coll/coll_algorithms.hpp"
#include "sched/entry_factory.hpp"

iccl_status_t iccl_coll_build_direct_allgatherv(iccl_sched* sched, const void* send_buf, size_t s_count,
                                                void* recv_buf, size_t* r_counts, iccl_datatype_internal_t dtype)
{
    LOG_DEBUG("build direct allgatherv");

    entry_factory::make_allgatherv_entry(sched, send_buf, s_count, recv_buf, r_counts, dtype);
    return iccl_status_success;
}

iccl_status_t iccl_coll_build_naive_allgatherv(iccl_sched* sched, const void* send_buf, size_t send_count,
                                               void* recv_buf, size_t* recv_counts, iccl_datatype_internal_t dtype)
{
    LOG_DEBUG("build naive allgatherv");

    size_t comm_size     = sched->coll_param.comm->size();
    size_t this_rank     = sched->coll_param.comm->rank();
    size_t dtype_size    = iccl_datatype_get_size(dtype);
    size_t* offsets      = static_cast<size_t*>(ICCL_MALLOC(comm_size * sizeof(size_t), "offsets"));
    iccl_status_t status = iccl_status_success;

    offsets[0] = 0;
    for (size_t rank_idx = 1; rank_idx < comm_size; ++rank_idx)
    {
        offsets[rank_idx] = offsets[rank_idx - 1] + recv_counts[rank_idx - 1] * dtype_size;
    }

    if (send_buf != recv_buf)
    {
        //out-of-place case
        entry_factory::make_copy_entry(sched, send_buf, static_cast<char*>(recv_buf) + offsets[this_rank],
                                       send_count, dtype);
    }

    for (size_t rank_idx = 0; rank_idx < sched->coll_param.comm->size(); ++rank_idx)
    {
        if (rank_idx != this_rank)
        {
            // send own buffer to other rank
            entry_factory::make_send_entry(sched, static_cast<char*>(recv_buf) + offsets[this_rank],
                                           send_count, dtype, rank_idx);
            // recv other's rank buffer
            entry_factory::make_recv_entry(sched, static_cast<char*>(recv_buf) + offsets[rank_idx],
                                           recv_counts[rank_idx], dtype, rank_idx);
        }
    }

    ICCL_FREE(offsets);
    return status;
}
