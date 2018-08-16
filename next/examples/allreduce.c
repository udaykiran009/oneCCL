#include "assert.h"
#include "math.h"
#include "mlsl_sched.h"
#include "stdio.h"
#include "string.h"
#include <sys/time.h>

#define COUNT 1048576
#define ITERS 1024
#define MLSL_CALL(expr)                        \
  do {                                         \
        status = expr;                         \
        assert(status == mlsl_status_success); \
  } while (0)

double when(void)
{
    struct timeval tv;
    static struct timeval tv_base;
    static int is_first = 1;

    if (gettimeofday(&tv, NULL)) {
        perror("gettimeofday");
        return 0;
    }

    if (is_first) {
        tv_base = tv;
        is_first = 0;
    }
    return (double)(tv.tv_sec - tv_base.tv_sec) * 1.0e6 + (double)(tv.tv_usec - tv_base.tv_usec);
}

int main()
{
    mlsl_sched_t sched;
    mlsl_request_t request;
    mlsl_status_t status = mlsl_status_success;
    size_t proc_idx, proc_count;
    double t1, t2, t;
    size_t idx, iter_idx;
    float send_buf[COUNT];
    float recv_buf[COUNT];

    MLSL_CALL(mlsl_init());
    MLSL_CALL(mlsl_sched_allreduce(send_buf, recv_buf, COUNT, mlsl_dtype_float, mlsl_reduction_sum, &sched));

    proc_idx = mlsl_get_proc_idx();
    proc_count = mlsl_get_proc_count();

    for (iter_idx = 0; iter_idx < ITERS; iter_idx++)
    {
        for (idx = 0; idx < COUNT; idx++)
        {
            send_buf[idx] = 1.0;
            recv_buf[idx] = 0.0;
        }

        t1 = when();

        MLSL_CALL(mlsl_sched_start(sched, &request));
        MLSL_CALL(mlsl_wait(request));

        t2 = when();
        t += (t2 - t1);

        float expected = proc_count;
        for (idx = 0; idx < COUNT; idx++)
        {
            if (recv_buf[idx] != expected)
            {
                printf("iter %zu, idx %zu, expected %f, got %f\n",
                       iter_idx, idx, expected, recv_buf[idx]);
                assert(0);
            }
        }
    }

    MLSL_CALL(mlsl_sched_free(sched));
    MLSL_CALL(mlsl_finalize());

    printf("[%zu] exited normally, avg allreduce time: %8.2lf us\n", proc_idx, t / ITERS);
    fflush(stdout);
}
