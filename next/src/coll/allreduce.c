#include "coll_algorithms.h"

mlsl_status_t mlsl_coll_build_rabenseifner_allreduce(mlsl_sched *sched, const void *send_buf, void *recv_buf,
                                                     size_t count, mlsl_data_type_t dtype, mlsl_reduction_t op)
{
    MLSL_LOG(DEBUG, "build Rabenseifner's allreduce");

    mlsl_status_t status = mlsl_status_success;
    int comm_size, rank, newrank, pof2, rem;
    int i, send_idx, recv_idx, last_idx, mask, newdst, dst, send_cnt, recv_cnt;
    void *tmp_buf = NULL;
    int *cnts = NULL, *disps = NULL;
    size_t dtype_size = mlsl_get_dtype_size(dtype);

    mlsl_comm *comm = global_data.comm;

    comm_size = comm->proc_count;
    rank = comm->proc_idx;

    tmp_buf = MLSL_MALLOC(count * dtype_size, "tmp_buf");
    mlsl_sched_add_persistent_memory(sched, mlsl_sched_memory_buffer, tmp_buf);

    /* copy local data into recv_buf */
    if (send_buf != recv_buf) {
        MLSL_CALL(mlsl_sched_add_copy(sched, send_buf, recv_buf, count, dtype));
        MLSL_CALL(mlsl_sched_add_barrier(sched));
    }

    /* get nearest power-of-two less than or equal to comm_size */
    pof2 = comm->pof2;

    rem = comm_size - pof2;

    /* In the non-power-of-two case, all even-numbered
     * processes of rank < 2*rem send their data to
     * (rank+1). These even-numbered processes no longer
     * participate in the algorithm until the very end. The
     * remaining processes form a nice power-of-two. */

    if (rank < 2 * rem) {
        if (rank % 2 == 0) {    /* even */
            MLSL_CALL(mlsl_sched_add_send(sched, recv_buf, count, dtype, rank + 1));
            MLSL_CALL(mlsl_sched_add_barrier(sched));

            /* temporarily set the rank to -1 so that this
             * process does not pariticipate in recursive
             * doubling */
            newrank = MLSL_INVALID_PROC_IDX;
        } else {        /* odd */
            MLSL_CALL(mlsl_sched_add_recv(sched, tmp_buf, count, dtype, rank - 1));
            MLSL_CALL(mlsl_sched_add_barrier(sched));

            /* do the reduction on received data. since the
             * ordering is right, it doesn't matter whether
             * the operation is commutative or not. */
            MLSL_CALL(mlsl_sched_add_reduce(sched, tmp_buf, count,
                                            recv_buf, NULL, dtype, op));
            MLSL_CALL(mlsl_sched_add_barrier(sched));

            /* change the rank */
            newrank = rank / 2;
        }
    } else      /* rank >= 2*rem */
        newrank = rank - rem;

    if (newrank != MLSL_INVALID_PROC_IDX) {
        /* for the reduce-scatter, calculate the count that
         * each process receives and the displacement within
         * the buffer */
        cnts = MLSL_MALLOC(pof2 * sizeof(int), "counts");
        disps = MLSL_MALLOC(pof2 * sizeof(int), "displacements");

        /* the cnts calculations assume this */
        MLSL_ASSERT_FMT(count >= pof2, "count %zu, pof2 %d", count, pof2);

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

            MLSL_ASSERTP(can_use_recv_reduce);

            if (can_use_recv_reduce) {
                MLSL_CALL(mlsl_sched_add_recv_reduce(sched,
                                                     ((char *) recv_buf + disps[recv_idx] * dtype_size),
                                                     recv_cnt, dtype, dst, op));
                MLSL_CALL(mlsl_sched_add_send(sched, ((char *) recv_buf + disps[send_idx] * dtype_size), send_cnt, dtype, dst));
                MLSL_CALL(mlsl_sched_add_barrier(sched));
            }

            else {
                /* Send data from recv_buf. Recv into tmp_buf */
                MLSL_CALL(mlsl_sched_add_recv(sched, ((char *) tmp_buf + disps[recv_idx] * dtype_size), recv_cnt, dtype, dst));
                /* sendrecv, no barrier here */
                MLSL_CALL(mlsl_sched_add_send(sched, ((char *) recv_buf + disps[send_idx] * dtype_size), send_cnt, dtype, dst));
                MLSL_CALL(mlsl_sched_add_barrier(sched));

                /* tmp_buf contains data received in this step.
                 * recv_buf contains data accumulated so far */

                /* This algorithm is used only for predefined ops
                 * and predefined ops are always commutative. */
                MLSL_CALL(mlsl_sched_add_reduce(sched, ((char *) tmp_buf + disps[recv_idx] * dtype_size), recv_cnt,
                                                ((char *) recv_buf + disps[recv_idx] * dtype_size), NULL, dtype, op));
                MLSL_CALL(mlsl_sched_add_barrier(sched));
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

            MLSL_CALL(mlsl_sched_add_recv(sched, ((char *) recv_buf + disps[recv_idx] * dtype_size), recv_cnt, dtype, dst));
            /* sendrecv, no barrier here */
            MLSL_CALL(mlsl_sched_add_send(sched, ((char *) recv_buf + disps[send_idx] * dtype_size), send_cnt, dtype, dst));
            MLSL_CALL(mlsl_sched_add_barrier(sched));

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
            MLSL_CALL(mlsl_sched_add_send(sched, recv_buf, count, dtype, rank - 1));
        } else {        /* even */
            MLSL_CALL(mlsl_sched_add_recv(sched, recv_buf, count, dtype, rank + 1));
        }
    }

    MLSL_FREE(cnts);
    MLSL_FREE(disps);

    return status;
}

mlsl_status_t mlsl_coll_build_recursive_doubling_allreduce(mlsl_sched *sched, const void *send_buf, void *recv_buf,
                                                           size_t count, mlsl_data_type_t dtype, mlsl_reduction_t op)
{
    MLSL_LOG(DEBUG, "build recursive_doubling allreduce");

    mlsl_status_t status = mlsl_status_success;

    int pof2, rem, comm_size, rank;
    int newrank, mask, newdst, dst;
    void *tmp_buf = NULL;

    mlsl_comm *comm = global_data.comm;
    comm_size = comm->proc_count;
    rank = comm->proc_idx;

    size_t dtype_size = mlsl_get_dtype_size(dtype);
    tmp_buf = MLSL_MALLOC(count * dtype_size, "tmp_buf");
    mlsl_sched_add_persistent_memory(sched, mlsl_sched_memory_buffer, tmp_buf);

    /* copy local data into recv_buf */
    if (send_buf != recv_buf) {
        MLSL_CALL(mlsl_sched_add_copy(sched, send_buf, recv_buf, count, dtype));
        MLSL_CALL(mlsl_sched_add_barrier(sched));
    }

    /* get nearest power-of-two less than or equal to comm_size */
    pof2 = comm->pof2;
    rem = comm_size - pof2;

    /* In the non-power-of-two case, all even-numbered
     * processes of rank < 2*rem send their data to
     * (rank+1). These even-numbered processes no longer
     * participate in the algorithm until the very end. The
     * remaining processes form a nice power-of-two. */

    if (rank < 2 * rem) {
        if (rank % 2 == 0) {    /* even */

            MLSL_CALL(mlsl_sched_add_send(sched, recv_buf, count, dtype, rank + 1));
            MLSL_CALL(mlsl_sched_add_barrier(sched));

            /* temporarily set the rank to -1 so that this
             * process does not pariticipate in recursive
             * doubling */
            newrank = -1;
        } else {        /* odd */

            MLSL_CALL(mlsl_sched_add_recv(sched, tmp_buf, count, dtype, rank - 1));
            MLSL_CALL(mlsl_sched_add_barrier(sched));

            /* do the reduction on received data. since the
             * ordering is right, it doesn't matter whether
             * the operation is commutative or not. */

            MLSL_CALL(mlsl_sched_add_reduce(sched, tmp_buf, count,
                                            recv_buf, NULL, dtype, op));
            MLSL_CALL(mlsl_sched_add_barrier(sched));

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
            MLSL_CALL(mlsl_sched_add_recv(sched, tmp_buf, count, dtype, dst));
            /* sendrecv, no barrier here */
            MLSL_CALL(mlsl_sched_add_send(sched, recv_buf, count, dtype, dst));
            MLSL_CALL(mlsl_sched_add_barrier(sched));

            /* tmp_buf contains data received in this step.
             * recv_buf contains data accumulated so far */
            MLSL_CALL(mlsl_sched_add_reduce(sched, tmp_buf, count,
                                                recv_buf, NULL, dtype, op));
            MLSL_CALL(mlsl_sched_add_barrier(sched));

            mask <<= 1;
        }
    }

    /* In the non-power-of-two case, all odd-numbered
     * processes of rank < 2*rem send the result to
     * (rank-1), the ranks who didn't participate above. */
    if (rank < 2 * rem) {
        if (rank % 2) { /* odd */
            MLSL_CALL(mlsl_sched_add_send(sched, recv_buf, count, dtype, rank - 1));
        } else {        /* even */
            MLSL_CALL(mlsl_sched_add_recv(sched, recv_buf, count, dtype, rank + 1));
        }
    }

    return status;
}
