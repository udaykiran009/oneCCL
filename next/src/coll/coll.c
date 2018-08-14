#include "coll.h"
#include "global.h"
#include "sched.h"
#include "utils.h"

mlsl_status_t mlsl_coll_build_ring_allreduce(mlsl_sched *sched, const void *send_buf, void *recv_buf,
                                             size_t count, mlsl_data_type_t dtype, mlsl_reduction_t op)
{
    MLSL_LOG(DEBUG, "build ring allreduce (out-of-place)");

    mlsl_status_t status = mlsl_status_success;

    size_t proc_count, proc_idx;
    int i, send_count, recv_count;
    size_t dtype_size;
    int send_idx, recv_idx;
    int count0, count1, src, dst;

    dtype_size = mlsl_get_dtype_size(dtype);
    proc_count = mlsl_get_proc_count();
    proc_idx = mlsl_get_proc_idx();

    count0 = count / proc_count;
    count1 = count - count0 * (proc_count - 1);
    src = (proc_count + proc_idx - 1) % proc_count;
    dst = (proc_idx + 1) % proc_count;

    if (proc_count == 1)
    {
        MLSL_CALL(mlsl_sched_add_copy(sched, send_buf, recv_buf, count, dtype));
        MLSL_CALL(mlsl_sched_add_barrier(sched));
        goto fn_exit;
    }

    /* do reduce-scatter */
    for (i = 0; i < proc_count - 1; i++)
    {
        send_idx = (proc_count + proc_idx - i) % proc_count;
        recv_idx = (proc_count + proc_idx - i - 1) % proc_count;
        send_count = (send_idx < proc_count - 1) ? count0 : count1;
        recv_count = (recv_idx < proc_count - 1) ? count0 : count1;

        if (i == 0) {
            MLSL_CALL(mlsl_sched_add_send(sched, (char *) send_buf + count0 * send_idx * dtype_size,
                                          send_count, dtype, dst));
        } else {
            MLSL_CALL(mlsl_sched_add_send(sched, (char *) recv_buf + count0 * send_idx * dtype_size,
                                          send_count, dtype, dst));
        }
        MLSL_CALL(mlsl_sched_add_recv(sched, (char *) recv_buf + count0 * recv_idx * dtype_size,
                                      recv_count, dtype, src));
        MLSL_CALL(mlsl_sched_add_barrier(sched));
        MLSL_CALL(mlsl_sched_add_reduce(sched,
                                        (char *) send_buf + count0 * recv_idx * dtype_size, recv_count,
                                        (char *) recv_buf + count0 * recv_idx * dtype_size, NULL,
                                        dtype, op));
        MLSL_CALL(mlsl_sched_add_barrier(sched));
    }

    /* now do the allgather */
    for (i = 0; i < proc_count - 1; i++)
    {
        send_idx = (proc_count + proc_idx - i + 1) % proc_count;
        recv_idx = (proc_count + proc_idx - i) % proc_count;
        send_count = (send_idx < proc_count - 1) ? count0 : count1;
        recv_count = (recv_idx < proc_count - 1) ? count0 : count1;

        /* Send/recv data from/to recv_buf */
        MLSL_CALL(mlsl_sched_add_send(sched, (char *) recv_buf + count0 * send_idx * dtype_size,
                                      send_count, dtype, dst));
        MLSL_CALL(mlsl_sched_add_recv(sched, (char *) recv_buf + count0 * recv_idx * dtype_size,
                                      recv_count, dtype, src));
        MLSL_CALL(mlsl_sched_add_barrier(sched));
    }

  fn_exit:
    MLSL_CALL(mlsl_sched_commit(sched));
    return status;
}

mlsl_status_t mlsl_coll_build_rabenseifner_allreduce(mlsl_sched *sched, const void *send_buf, void *recv_buf,
                                                     size_t count, mlsl_data_type_t dtype, mlsl_reduction_t op)
{
    MLSL_LOG(DEBUG, "build Rabenseifner's allreduce");

    mlsl_status_t status = mlsl_status_success;
    size_t proc_count, proc_idx, newproc_idx, pof2, rem;
    size_t i, send_idx, recv_idx, last_idx, mask, newdst, dst, send_cnt, recv_cnt;
    void *tmp_buf = NULL;
    size_t *cnts = NULL, *disps = NULL;
    size_t dtype_size = mlsl_get_dtype_size(dtype);

    mlsl_comm *comm = global_data.comm;

    proc_count = comm->proc_count;
    proc_idx = comm->proc_idx;

    tmp_buf = MLSL_MALLOC(count * dtype_size, "tmp_buf");
    mlsl_sched_add_persistent_memory(sched, mlsl_sched_memory_buffer, tmp_buf);

    /* copy local data into recv_buf */
    if (send_buf != recv_buf) {
        MLSL_CALL(mlsl_sched_add_copy(sched, send_buf, recv_buf, count, dtype));
        MLSL_CALL(mlsl_sched_add_barrier(sched));
    }

    /* get nearest power-of-two less than or equal to proc_count */
    pof2 = comm->pof2;

    rem = proc_count - pof2;

    /* In the non-power-of-two case, all even-numbered
     * processes of proc_idx < 2*rem send their data to
     * (proc_idx+1). These even-numbered processes no longer
     * participate in the algorithm until the very end. The
     * remaining processes form a nice power-of-two. */

    if (proc_idx < 2 * rem) {
        if (proc_idx % 2 == 0) {    /* even */
            MLSL_CALL(mlsl_sched_add_send(sched, recv_buf, count, dtype, proc_idx + 1));
            MLSL_CALL(mlsl_sched_add_barrier(sched));

            /* temporarily set the proc_idx to -1 so that this
             * process does not pariticipate in recursive
             * doubling */
            newproc_idx = -1;
        } else {        /* odd */
            MLSL_CALL(mlsl_sched_add_recv(sched, tmp_buf, count, dtype, proc_idx - 1));
            MLSL_CALL(mlsl_sched_add_barrier(sched));

            /* do the reduction on received data. since the
             * ordering is right, it doesn't matter whether
             * the operation is commutative or not. */
            MLSL_CALL(mlsl_sched_add_reduce(sched, tmp_buf, count,
                                            recv_buf, NULL, dtype, op));
            MLSL_CALL(mlsl_sched_add_barrier(sched));

            /* change the proc_idx */
            newproc_idx = proc_idx / 2;
        }
    } else      /* proc_idx >= 2*rem */
        newproc_idx = proc_idx - rem;

    if (newproc_idx != -1) {
        /* for the reduce-scatter, calculate the count that
         * each process receives and the displacement within
         * the buffer */
        cnts = MLSL_MALLOC(pof2 * sizeof(size_t), "counts");
        disps = MLSL_MALLOC(pof2 * sizeof(size_t), "displacements");

        /* the cnts calculations assume this */
        MLSL_ASSERTP(count >= pof2);

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
            newdst = newproc_idx ^ mask;
            /* find real proc_idx of dest */
            dst = (newdst < rem) ? newdst * 2 + 1 : newdst + rem;

            send_cnt = recv_cnt = 0;
            if (newproc_idx < newdst) {
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
            newdst = newproc_idx ^ mask;
            /* find real proc_idx of dest */
            dst = (newdst < rem) ? newdst * 2 + 1 : newdst + rem;

            send_cnt = recv_cnt = 0;
            if (newproc_idx < newdst) {
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

            if (newproc_idx > newdst)
                send_idx = recv_idx;

            mask >>= 1;
        }
    }

    /* In the non-power-of-two case, all odd-numbered
     * processes of proc_idx < 2*rem send the result to
     * (proc_idx-1), the proc_idxs who didn't participate above. */
    if (proc_idx < 2 * rem) {
        if (proc_idx % 2) { /* odd */
            MLSL_CALL(mlsl_sched_add_send(sched, recv_buf, count, dtype, proc_idx - 1));
        } else {        /* even */
            MLSL_CALL(mlsl_sched_add_recv(sched, recv_buf, count, dtype, proc_idx + 1));
        }
    }

    MLSL_FREE(cnts);
    MLSL_FREE(disps);

    return status;
}

mlsl_status_t mlsl_coll_build_allreduce(
    mlsl_sched *sched,
    void *send_buf,
    void *recv_buf,
    size_t count,
    mlsl_data_type_t dtype,
    mlsl_reduction_t reduction)
{
    mlsl_status_t status;

    // TODO: add algorithm selection logic
    //MLSL_CALL(mlsl_coll_build_ring_allreduce(sched, send_buf, recv_buf, count, dtype, reduction));
    MLSL_CALL(mlsl_coll_build_rabenseifner_allreduce(sched, send_buf, recv_buf, count, dtype, reduction));

    return status;
}

mlsl_status_t mlsl_allreduce(
    void *send_buf,
    void *recv_buf,
    size_t count,
    mlsl_data_type_t dtype,
    mlsl_reduction_t reduction,
    mlsl_request **req)
{
    mlsl_status_t status;
    mlsl_sched *sched;

    MLSL_CALL(mlsl_sched_create(&sched));
    MLSL_CALL(mlsl_coll_build_allreduce(sched, send_buf, recv_buf, count, dtype, reduction));
    MLSL_CALL(mlsl_sched_commit_with_type(sched, mlsl_sched_non_persistent));
    MLSL_CALL(mlsl_sched_start(sched, req));

    return status;
}
