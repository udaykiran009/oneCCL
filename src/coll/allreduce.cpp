#include "coll/coll_algorithms.hpp"
#include "sched/entry_factory.hpp"

mlsl_status_t mlsl_coll_build_rabenseifner_allreduce(mlsl_sched *sched, const void *send_buf, void *recv_buf,
                                                     size_t count, mlsl_datatype_internal_t dtype, mlsl_reduction_t op)
{
    MLSL_LOG(DEBUG, "build Rabenseifner's allreduce");

    mlsl_status_t status = mlsl_status_success;
    int comm_size, rank, newrank, pof2, rem;
    int i, send_idx, recv_idx, last_idx, mask, newdst, dst, send_cnt, recv_cnt;
    void *tmp_buf = NULL;
    int *cnts = NULL, *disps = NULL;
    size_t dtype_size = mlsl_datatype_get_size(dtype);

    mlsl_comm *comm = sched->coll_param.comm;

    comm_size = comm->size();
    rank = comm->rank();

    tmp_buf = sched->alloc_buffer(count * dtype_size);

    /* copy local data into recv_buf */
    if (send_buf != recv_buf) {
        entry_factory::make_copy_entry(sched, send_buf, recv_buf, count, dtype);
        sched->add_barrier();
    }

    /* get nearest power-of-two less than or equal to comm_size */
    pof2 = comm->pof2();

    rem = comm_size - pof2;

    /* In the non-power-of-two case, all even-numbered
     * processes of rank < 2*rem send their data to
     * (rank+1). These even-numbered processes no longer
     * participate in the algorithm until the very end. The
     * remaining processes form a nice power-of-two. */

    if (rank < 2 * rem) {
        if (rank % 2 == 0) {    /* even */
            entry_factory::make_send_entry(sched, recv_buf, count, dtype, rank + 1);
            sched->add_barrier();

            /* temporarily set the rank to -1 so that this
             * process does not pariticipate in recursive
             * doubling */
            newrank = MLSL_INVALID_PROC_IDX;
        } else {        /* odd */
            entry_factory::make_recv_entry(sched, tmp_buf, count, dtype, rank - 1);
            sched->add_barrier();

            /* do the reduction on received data. since the
             * ordering is right, it doesn't matter whether
             * the operation is commutative or not. */
            entry_factory::make_reduce_entry(sched, tmp_buf, count,
                                             recv_buf, NULL, dtype, op);
            sched->add_barrier();

            /* change the rank */
            newrank = rank / 2;
        }
    } else      /* rank >= 2*rem */
        newrank = rank - rem;

    if (newrank != MLSL_INVALID_PROC_IDX) {
        /* for the reduce-scatter, calculate the count that
         * each process receives and the displacement within
         * the buffer */
        cnts = static_cast<int*>(MLSL_MALLOC(pof2 * sizeof(int), "counts"));
        disps = static_cast<int*>(MLSL_MALLOC(pof2 * sizeof(int), "displacements"));

        /* the cnts calculations assume this */
        MLSL_ASSERT(count >= static_cast<size_t>(pof2), "count %zu, pof2 %d", count, pof2);

        for (i = 0; i < (pof2 - 1); i++)
            cnts[i] = count / pof2;
        cnts[pof2 - 1] = count - (count / pof2) * (pof2 - 1);

        disps[0] = 0;
        for (i = 1; i < pof2; i++)
            disps[i] = disps[i - 1] + cnts[i - 1];

        mask = 0x1;
        send_idx = recv_idx = 0;
        last_idx = pof2;
        while (mask < pof2) {
            newdst = newrank ^ mask;
            /* find real rank of dest */
            dst = (newdst < rem) ? newdst * 2 + 1 : newdst + rem;

            send_cnt = recv_cnt = 0;
            if (newrank < newdst) {
                send_idx = recv_idx + pof2 / (mask * 2);
                for (i = send_idx; i < last_idx; i++)
                    send_cnt += cnts[i];
                for (i = recv_idx; i < send_idx; i++)
                    recv_cnt += cnts[i];
            } else {
                recv_idx = send_idx + pof2 / (mask * 2);
                for (i = send_idx; i < recv_idx; i++)
                    send_cnt += cnts[i];
                for (i = recv_idx; i < last_idx; i++)
                    recv_cnt += cnts[i];
            }

            int can_use_recv_reduce = 0;
            char* buf1 = ((char *) recv_buf + disps[send_idx] * dtype_size);
            char* buf2 = ((char *) recv_buf + disps[recv_idx] * dtype_size);
            if (buf1 != buf2 && ((buf1 + send_cnt * dtype_size <= buf2) || (buf2 + recv_cnt * dtype_size <= buf1)))
                can_use_recv_reduce = 1;

            MLSL_ASSERT(can_use_recv_reduce, "");

            if (can_use_recv_reduce) {
                entry_factory::make_recv_reduce_entry(sched,
                                                      ((char*) recv_buf + disps[recv_idx] * dtype_size),
                                                      recv_cnt, NULL, dtype, op, dst, NULL);
                entry_factory::make_send_entry(sched, ((char*) recv_buf + disps[send_idx] * dtype_size), send_cnt, dtype, dst);
                sched->add_barrier();
            }

            else {
                /* Send data from recv_buf. Recv into tmp_buf */
                entry_factory::make_recv_entry(sched, ((char*) tmp_buf + disps[recv_idx] * dtype_size), recv_cnt, dtype, dst);
                /* sendrecv, no barrier here */
                entry_factory::make_send_entry(sched, ((char*) recv_buf + disps[send_idx] * dtype_size), send_cnt, dtype, dst);
                sched->add_barrier();

                /* tmp_buf contains data received in this step.
                 * recv_buf contains data accumulated so far */

                /* This algorithm is used only for predefined ops
                 * and predefined ops are always commutative. */
                entry_factory::make_reduce_entry(sched, ((char *) tmp_buf + disps[recv_idx] * dtype_size), recv_cnt,
                                                 ((char *) recv_buf + disps[recv_idx] * dtype_size), NULL, dtype, op);
                sched->add_barrier();
            }

            /* update send_idx for next iteration */
            send_idx = recv_idx;
            mask <<= 1;

            /* update last_idx, but not in last iteration
             * because the value is needed in the allgather
             * step below. */
            if (mask < pof2)
                last_idx = recv_idx + pof2 / mask;
        }

        /* now do the allgather */

        mask >>= 1;
        while (mask > 0) {
            newdst = newrank ^ mask;
            /* find real rank of dest */
            dst = (newdst < rem) ? newdst * 2 + 1 : newdst + rem;

            send_cnt = recv_cnt = 0;
            if (newrank < newdst) {
                /* update last_idx except on first iteration */
                if (mask != pof2 / 2)
                    last_idx = last_idx + pof2 / (mask * 2);

                recv_idx = send_idx + pof2 / (mask * 2);
                for (i = send_idx; i < recv_idx; i++)
                    send_cnt += cnts[i];
                for (i = recv_idx; i < last_idx; i++)
                    recv_cnt += cnts[i];
            } else {
                recv_idx = send_idx - pof2 / (mask * 2);
                for (i = send_idx; i < last_idx; i++)
                    send_cnt += cnts[i];
                for (i = recv_idx; i < send_idx; i++)
                    recv_cnt += cnts[i];
            }

            entry_factory::make_recv_entry(sched, ((char*) recv_buf + disps[recv_idx] * dtype_size), recv_cnt, dtype, dst);
            /* sendrecv, no barrier here */
            entry_factory::make_send_entry(sched, ((char*) recv_buf + disps[send_idx] * dtype_size), send_cnt, dtype, dst);
            sched->add_barrier();

            if (newrank > newdst)
                send_idx = recv_idx;

            mask >>= 1;
        }
    }

    /* In the non-power-of-two case, all odd-numbered
     * processes of rank < 2*rem send the result to
     * (rank-1), the ranks who didn't participate above. */
    if (rank < 2 * rem) {
        if (rank % 2) { /* odd */
            entry_factory::make_send_entry(sched, recv_buf, count, dtype, rank - 1);
        } else {        /* even */
            entry_factory::make_recv_entry(sched, recv_buf, count, dtype, rank + 1);
        }
    }

    MLSL_FREE(cnts);
    MLSL_FREE(disps);

    return status;
}

mlsl_status_t mlsl_coll_build_recursive_doubling_allreduce(mlsl_sched *sched, const void *send_buf, void *recv_buf,
                                                           size_t count, mlsl_datatype_internal_t dtype, mlsl_reduction_t op)
{
    MLSL_LOG(DEBUG, "build recursive_doubling allreduce");

    mlsl_status_t status = mlsl_status_success;

    int pof2, rem, comm_size, rank;
    int newrank, mask, newdst, dst;
    void *tmp_buf = NULL;

    mlsl_comm *comm = sched->coll_param.comm;
    comm_size = comm->size();
    rank = comm->rank();

    size_t dtype_size = mlsl_datatype_get_size(dtype);
    tmp_buf = sched->alloc_buffer(count * dtype_size);

    /* copy local data into recv_buf */
    if (send_buf != recv_buf) {
        entry_factory::make_copy_entry(sched, send_buf, recv_buf, count, dtype);
        sched->add_barrier();
    }

    /* get nearest power-of-two less than or equal to comm_size */
    pof2 = comm->pof2();
    rem = comm_size - pof2;

    /* In the non-power-of-two case, all even-numbered
     * processes of rank < 2*rem send their data to
     * (rank+1). These even-numbered processes no longer
     * participate in the algorithm until the very end. The
     * remaining processes form a nice power-of-two. */

    if (rank < 2 * rem) {
        if (rank % 2 == 0) {    /* even */
            entry_factory::make_send_entry(sched, recv_buf, count, dtype, rank + 1);
            sched->add_barrier();

            /* temporarily set the rank to -1 so that this
             * process does not pariticipate in recursive
             * doubling */
            newrank = -1;
        } else {        /* odd */
            entry_factory::make_recv_entry(sched, tmp_buf, count, dtype, rank - 1);
            sched->add_barrier();

            /* do the reduction on received data. since the
             * ordering is right, it doesn't matter whether
             * the operation is commutative or not. */

            entry_factory::make_reduce_entry(sched, tmp_buf, count,
                                             recv_buf, NULL, dtype, op);
            sched->add_barrier();

            /* change the rank */
            newrank = rank / 2;
        }
    } else      /* rank >= 2*rem */
        newrank = rank - rem;

    if (newrank != -1) {
        mask = 0x1;
        while (mask < pof2) {
            newdst = newrank ^ mask;
            /* find real rank of dest */
            dst = (newdst < rem) ? newdst * 2 + 1 : newdst + rem;

            /* Send the most current data, which is in recv_buf. Recv
             * into tmp_buf */
            entry_factory::make_recv_entry(sched, tmp_buf, count, dtype, dst);
            /* sendrecv, no barrier here */
            entry_factory::make_send_entry(sched, recv_buf, count, dtype, dst);
            sched->add_barrier();

            /* tmp_buf contains data received in this step.
             * recv_buf contains data accumulated so far */
            entry_factory::make_reduce_entry(sched, tmp_buf, count,
                                             recv_buf, NULL, dtype, op);
            sched->add_barrier();

            mask <<= 1;
        }
    }

    /* In the non-power-of-two case, all odd-numbered
     * processes of rank < 2*rem send the result to
     * (rank-1), the ranks who didn't participate above. */
    if (rank < 2 * rem) {
        if (rank % 2) { /* odd */
            entry_factory::make_send_entry(sched, recv_buf, count, dtype, rank - 1);
        } else {        /* even */
            entry_factory::make_recv_entry(sched, recv_buf, count, dtype, rank + 1);
        }
        sched->add_barrier();
    }

    return status;
}

mlsl_status_t mlsl_coll_build_starlike_allreduce(mlsl_sched *sched, const void *send_buf, void *recv_buf,
                                                 size_t count, mlsl_datatype_internal_t dtype, mlsl_reduction_t op)
{
    MLSL_LOG(DEBUG, "build starlike allreduce");

    mlsl_status_t status = mlsl_status_success;
    size_t comm_size = sched->coll_param.comm->size();
    size_t this_rank = sched->coll_param.comm->rank();
    size_t* buffer_counts = static_cast<size_t*>(MLSL_MALLOC(comm_size * sizeof(size_t), "buffer_count"));
    size_t* buffer_offsets = static_cast<size_t*>(MLSL_MALLOC(comm_size * sizeof(size_t), "buffer_offsets"));
    size_t dtype_size = mlsl_datatype_get_size(dtype);
    void *tmp_buf = NULL;

    // find counts and offsets for each rank
    size_t common_buffer_count = count / comm_size;
    for (size_t rank_idx = 0; rank_idx < comm_size; ++ rank_idx)
    {
        buffer_counts[rank_idx] = common_buffer_count;
        buffer_offsets[rank_idx] = rank_idx * buffer_counts[rank_idx] * dtype_size;
    }
    buffer_counts[comm_size - 1] += count % comm_size;

    // buffer to receive and reduce parts related to the current rank
    size_t this_rank_buf_size = buffer_counts[this_rank] * dtype_size;
    tmp_buf = sched->alloc_buffer(this_rank_buf_size * (comm_size - 1));

    // copy local data into recv_buf
    if (send_buf != recv_buf) {
        entry_factory::make_copy_entry(sched, send_buf, recv_buf, count, dtype);
        sched->add_barrier();
    }

    size_t tmp_buf_recv_idx = 0;
    for (size_t rank_idx = 0; rank_idx < comm_size; ++rank_idx)
    {
        if (rank_idx != this_rank)
        {
            // send buffer to others
            entry_factory::make_send_entry(sched, static_cast<char*>(recv_buf) + buffer_offsets[rank_idx],
                                           buffer_counts[rank_idx], dtype, rank_idx);

            // recv part of buffer from others and perform reduce
            entry_factory::make_recv_reduce_entry(sched,
                                                  static_cast<char*>(recv_buf) + buffer_offsets[this_rank],
                                                  buffer_counts[this_rank], NULL,
                                                  dtype, op, rank_idx,
                                                  static_cast<char*>(tmp_buf) + this_rank_buf_size * tmp_buf_recv_idx);
            ++tmp_buf_recv_idx;
        }
    }

    sched->add_barrier();

    // allgatherv
    MLSL_CALL(mlsl_coll_build_naive_allgatherv(sched, recv_buf, buffer_counts[this_rank],
                                               recv_buf, buffer_counts, dtype));

    MLSL_FREE(buffer_counts);
    MLSL_FREE(buffer_offsets);

    return status;
}

mlsl_status_t mlsl_coll_build_ring_allreduce(mlsl_sched *sched, const void *send_buf, void *recv_buf,
                                             size_t count, mlsl_datatype_internal_t dtype, mlsl_reduction_t op)
{
    int inplace = (send_buf == recv_buf) ? 1 : 0;
    MLSL_LOG(DEBUG, "build ring allreduce (%s)", (inplace) ? "in-place" : "out-of-place");

    mlsl_status_t status = mlsl_status_success;
    size_t comm_size, rank;
    mlsl_comm *comm = sched->coll_param.comm;
    size_t dtype_size = mlsl_datatype_get_size(dtype);
    size_t idx = 0;
    void* tmp_buf = NULL;
    size_t src, dst;

    comm_size = comm->size();
    rank = comm->rank();

    MLSL_THROW_IF_NOT(sched && send_buf && recv_buf, "incorrect values, sched %p send %p recv %p",
                                                      sched, send_buf, recv_buf);

    if (comm_size == 1)
    {
        if (!inplace) {
            entry_factory::make_copy_entry(sched, send_buf, recv_buf, count, dtype);
            sched->add_barrier();
        }
        return mlsl_status_success;
    }

    if (inplace)
        tmp_buf = sched->alloc_buffer(count * dtype_size);

    src = (comm_size + rank - 1) % comm_size;
    dst = (comm_size + rank + 1) % comm_size;

    size_t block_idx = rank; // start send with 'rank' block and recv with 'rank-1' block and move blocks left
    size_t main_block_count = count / comm_size;
    size_t last_block_count = main_block_count + count % comm_size;
    size_t send_block_idx, recv_block_idx;
    size_t send_block_count, recv_block_count;
    size_t send_block_offset, recv_block_offset;
    void* sbuf, *rbuf, *reduce_inout_buf;
    const void* reduce_in_buf;

    /* reduce-scatter */
    for (idx = 0; idx < (comm_size - 1); idx++)
    {
        send_block_idx = block_idx;
        recv_block_idx = (comm_size + block_idx - 1) % comm_size;
        send_block_count = (send_block_idx == (comm_size - 1)) ? last_block_count : main_block_count;
        recv_block_count = (recv_block_idx == (comm_size - 1)) ? last_block_count : main_block_count;
        send_block_offset = main_block_count * dtype_size * send_block_idx;
        recv_block_offset = main_block_count * dtype_size * recv_block_idx;
        if (inplace)
        {
            sbuf = recv_buf;
            rbuf = tmp_buf;
            reduce_in_buf = tmp_buf;
            reduce_inout_buf = recv_buf;
        }
        else
        {
            sbuf = (idx == 0) ? (void*)send_buf : recv_buf;
            rbuf = recv_buf;
            reduce_in_buf = send_buf;
            reduce_inout_buf = recv_buf;
        }
        sbuf = (char*)sbuf + send_block_offset;
        rbuf = (char*)rbuf + recv_block_offset;
        reduce_in_buf = (char*)reduce_in_buf + recv_block_offset;
        reduce_inout_buf = (char*)reduce_inout_buf + recv_block_offset;

        entry_factory::make_send_entry(sched, sbuf, send_block_count,
                                       dtype, dst);
        entry_factory::make_recv_entry(sched, rbuf, recv_block_count,
                                       dtype, src);
        sched->add_barrier();
        entry_factory::make_reduce_entry(sched, reduce_in_buf, recv_block_count,
                                         reduce_inout_buf, NULL, dtype, op);
        sched->add_barrier();

        block_idx = (comm_size + block_idx - 1) % comm_size; // move left
    }

    /* allgather */
    for (idx = 0; idx < (comm_size - 1); idx++)
    {
        send_block_idx = block_idx;
        recv_block_idx = (comm_size + block_idx - 1) % comm_size;
        send_block_count = (send_block_idx == (comm_size - 1)) ? last_block_count : main_block_count;
        recv_block_count = (recv_block_idx == (comm_size - 1)) ? last_block_count : main_block_count;
        send_block_offset = main_block_count * dtype_size * send_block_idx;
        recv_block_offset = main_block_count * dtype_size * recv_block_idx;
        sbuf = (char*)recv_buf + send_block_offset;
        rbuf = (char*)recv_buf + recv_block_offset;

        entry_factory::make_send_entry(sched, sbuf, send_block_count,
                                       dtype, dst);
        entry_factory::make_recv_entry(sched, rbuf, recv_block_count,
                                       dtype, src);
        sched->add_barrier();

        block_idx = (comm_size + block_idx - 1) % comm_size; // move left
    }

    return status;
}
