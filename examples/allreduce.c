#include "base.h"

#define RUN_COLLECTIVE(start_cmd, name)                                    \
  do {                                                                     \
      t = 0;                                                               \
      for (iter_idx = 0; iter_idx < ITERS; iter_idx++)                     \
      {                                                                    \
          for (idx = 0; idx < COUNT; idx++)                                \
          {                                                                \
              send_buf[idx] = (float)rank;                                 \
              recv_buf[idx] = 0.0;                                         \
          }                                                                \
          t1 = when();                                                     \
          MLSL_CALL(start_cmd);                                            \
          MLSL_CALL(mlsl_wait(request));                                   \
          t2 = when();                                                     \
          t += (t2 - t1);                                                  \
      }                                                                    \
      mlsl_barrier(NULL);                                                  \
      float expected = (size - 1) * ((float)size / 2);                     \
      for (idx = 0; idx < COUNT; idx++)                                    \
      {                                                                    \
          if (recv_buf[idx] != expected)                                   \
          {                                                                \
              printf("iter %zu, idx %zu, expected %f, got %f\n",           \
                      iter_idx, idx, expected, recv_buf[idx]);             \
              ASSERT(0, "unexpected value");                               \
          }                                                                \
      }                                                                    \
      printf("[%zu] avg %s time: %8.2lf us\n", rank, name, t / ITERS);     \
      fflush(stdout);                                                      \
  } while (0)

int main()
{
    float send_buf[COUNT];
    float recv_buf[COUNT];

    test_init();

    coll_attr.to_cache = 0;
    RUN_COLLECTIVE(mlsl_allreduce(send_buf, recv_buf, COUNT, mlsl_dtype_float, mlsl_reduction_sum, &coll_attr, NULL, &request),
                   "warmup_allreduce");

    coll_attr.to_cache = 1;
    RUN_COLLECTIVE(mlsl_allreduce(send_buf, recv_buf, COUNT, mlsl_dtype_float, mlsl_reduction_sum, &coll_attr, NULL, &request),
                   "persistent_allreduce");

    coll_attr.to_cache = 0;
    RUN_COLLECTIVE(mlsl_allreduce(send_buf, recv_buf, COUNT, mlsl_dtype_float, mlsl_reduction_sum, &coll_attr, NULL, &request),
                   "regular_allreduce");

    test_finalize();

    return 0;
}
