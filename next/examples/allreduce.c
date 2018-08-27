#include "base.h"

#define RUN_COLLECTIVE(start_cmd, name)                                    \
  do {                                                                     \
      t = 0;                                                               \
      for (iter_idx = 0; iter_idx < ITERS; iter_idx++)                     \
      {                                                                    \
          for (idx = 0; idx < COUNT; idx++)                                \
          {                                                                \
              send_buf[idx] = 1.0;                                         \
              recv_buf[idx] = 0.0;                                         \
          }                                                                \
          t1 = when();                                                     \
          MLSL_CALL(start_cmd);                                            \
          MLSL_CALL(mlsl_wait(request));                                   \
          t2 = when();                                                     \
          t += (t2 - t1);                                                  \
      }                                                                    \
      mlsl_barrier();                                                      \
      float expected = proc_count;                                         \
      for (idx = 0; idx < COUNT; idx++)                                    \
      {                                                                    \
          if (recv_buf[idx] != expected)                                   \
          {                                                                \
              printf("iter %zu, idx %zu, expected %f, got %f\n",           \
                      iter_idx, idx, expected, recv_buf[idx]);             \
              assert(0);                                                   \
          }                                                                \
      }                                                                    \
      printf("[%zu] avg %s time: %8.2lf us\n", proc_idx, name, t / ITERS); \
      fflush(stdout);                                                      \
  } while (0)

int main()
{
    mlsl_status_t status = mlsl_status_success;
    float send_buf[COUNT];
    float recv_buf[COUNT];

    MLSL_CALL(mlsl_init());

    proc_idx = mlsl_get_proc_idx();
    proc_count = mlsl_get_proc_count();

    MLSL_CALL(mlsl_sched_allreduce(send_buf, recv_buf, COUNT, mlsl_dtype_float, mlsl_reduction_sum, &sched));
    MLSL_CALL(mlsl_sched_commit(sched));
    RUN_COLLECTIVE(mlsl_sched_start(sched, &request), "persistent_allreduce");
    MLSL_CALL(mlsl_sched_free(sched));

    RUN_COLLECTIVE(mlsl_allreduce(send_buf, recv_buf, COUNT, mlsl_dtype_float, mlsl_reduction_sum, &request),
                   "regular_allreduce");

    MLSL_CALL(mlsl_finalize());

    return 0;
}
