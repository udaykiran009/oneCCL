#include "base.h"

#define RUN_COLLECTIVE(start_cmd, name)                                    \
  do {                                                                     \
      t = 0;                                                               \
      for (iter_idx = 0; iter_idx < ITERS; iter_idx++)                     \
      {                                                                    \
          for (idx = 0; idx < COUNT; idx++)                                \
          {                                                                \
              send_buf[idx] = (float)proc_idx;                             \
              recv_buf[idx] = 0.0;                                         \
          }                                                                \
          t1 = when();                                                     \
          MLSL_CALL(start_cmd);                                            \
          MLSL_CALL(mlsl_wait(request));                                   \
          t2 = when();                                                     \
          t += (t2 - t1);                                                  \
      }                                                                    \
      mlsl_barrier();                                                      \
      float expected = (proc_count - 1) * ((float)proc_count / 2);         \
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
    float send_buf[COUNT];
    float recv_buf[COUNT];

    MLSL_CALL(mlsl_init());

    proc_idx = mlsl_get_proc_idx();
    proc_count = mlsl_get_proc_count();

    coll_attr.to_cache = 1;
    RUN_COLLECTIVE(mlsl_allreduce(send_buf, recv_buf, COUNT, mlsl_dtype_float, mlsl_reduction_sum, &coll_attr, &request),
                   "persistent_allreduce");

    coll_attr.to_cache = 0;
    RUN_COLLECTIVE(mlsl_allreduce(send_buf, recv_buf, COUNT, mlsl_dtype_float, mlsl_reduction_sum, &coll_attr, &request),
                   "regular_allreduce");

    MLSL_CALL(mlsl_finalize());

    return 0;
}
