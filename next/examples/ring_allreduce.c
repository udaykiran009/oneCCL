#include "assert.h"
#include "mlsl_sched.h"
#include "stdio.h"

#define COUNT 1024
#define MLSL_CALL(expr)                        \
  do {                                         \
        status = expr;                         \
        assert(status == mlsl_status_success); \
  } while (0)

mlsl_status_t build_ring_allreduce(mlsl_sched_t sched, const void *send_buf, void *recv_buf,
                                   size_t count, mlsl_data_type_t dtype, mlsl_reduction_t op)
{
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
        MLSL_CALL(mlsl_sched_add_reduce(sched, (char *) send_buf + count0 * recv_idx * dtype_size,
                                        (char *) recv_buf + count0 * recv_idx * dtype_size,
                                        recv_count, dtype, op));
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

int main()
{
    mlsl_sched_t sched;
    mlsl_request_t request;
    mlsl_status_t status = mlsl_status_success;
    size_t proc_count;

    float send_buf[COUNT] = { 1.0 };
    float recv_buf[COUNT] = { 0.0 };

    MLSL_CALL(mlsl_init());

    MLSL_CALL(mlsl_sched_create(&sched));

    MLSL_CALL(build_ring_allreduce(sched, send_buf, recv_buf, COUNT,
                                   mlsl_dtype_float, mlsl_reduction_sum));

    MLSL_CALL(mlsl_sched_start(sched, &request));

    MLSL_CALL(mlsl_wait(request));

    size_t idx;
    proc_count = mlsl_get_proc_count();
    for (idx = 0; idx < COUNT; idx++)
    {
        assert(recv_buf[idx] == (float)proc_count);
    }

    MLSL_CALL(mlsl_sched_free(sched));

    MLSL_CALL(mlsl_finalize());
}
