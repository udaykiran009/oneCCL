#include "coll_algorithms.h"

mlsl_status_t mlsl_coll_build_naive_allgatherv(mlsl_sched* sched, const void* send_buf, size_t send_count,
                                               void* recv_buf, size_t* recv_counts, mlsl_data_type_t dtype)
{
    MLSL_LOG(DEBUG, "build naive allgatherv");

    size_t comm_size     = global_data.comm->proc_count;
    size_t this_rank     = global_data.comm->proc_idx;
    size_t dtype_size    = mlsl_get_dtype_size(dtype);
    size_t* offsets      = MLSL_MALLOC(comm_size * sizeof(size_t), "offsets");
    mlsl_status_t status = mlsl_status_success;

    offsets[0] = 0;
    for (size_t rank_idx = 1; rank_idx < comm_size; ++rank_idx)
    {
        offsets[rank_idx] = offsets[rank_idx - 1] + recv_counts[rank_idx - 1] * dtype_size;
    }

    if (send_buf != recv_buf)
    {
        //out-of-place case
        MLSL_CALL(mlsl_sched_add_copy(sched, send_buf, recv_buf + offsets[this_rank], send_count, dtype));
    }

    for (size_t rank_idx = 0; rank_idx < global_data.comm->proc_count; ++rank_idx)
    {
        if (rank_idx != this_rank)
        {
            // send own buffer to other rank
            MLSL_CALL(mlsl_sched_add_send(sched, recv_buf + offsets[this_rank], send_count, dtype, rank_idx));
            // recv other's rank buffer
            MLSL_CALL(mlsl_sched_add_recv(sched, recv_buf + offsets[rank_idx], recv_counts[rank_idx], dtype, rank_idx));
        }
    }

    MLSL_FREE(offsets);

    return status;
}
