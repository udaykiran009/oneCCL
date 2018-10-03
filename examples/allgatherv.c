#include "base.h"

#define RUN_COLLECTIVE(start_cmd, name)                                    \
  do {                                                                     \
      t = 0;                                                               \
      float expected = 1.0;                                                \
      for (iter_idx = 0; iter_idx < ITERS; iter_idx++)                     \
      {                                                                    \
          for (idx = 0; idx < COUNT; idx++)                                \
              send_buf[idx] = expected;                                    \
          for (idx = 0; idx < proc_count * COUNT; idx++)                   \
              recv_buf[idx] = 0.0;                                         \
          t1 = when();                                                     \
          MLSL_CALL(start_cmd);                                            \
          MLSL_CALL(mlsl_wait(request));                                   \
          t2 = when();                                                     \
          t += (t2 - t1);                                                  \
      }                                                                    \
      mlsl_barrier();                                                      \
      for (idx = 0; idx < proc_count * COUNT; idx++)                       \
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
    float *recv_buf;
    size_t *recv_counts;

    MLSL_CALL(mlsl_init());

    proc_idx = mlsl_get_proc_idx();
    proc_count = mlsl_get_proc_count();

    recv_buf = malloc(proc_count * COUNT * mlsl_get_dtype_size(mlsl_dtype_float));
    recv_counts = malloc(proc_count * sizeof(size_t));

    for (idx = 0; idx < proc_count; idx++)
        recv_counts[idx] = COUNT;

    coll_attr.to_cache = 1;
    RUN_COLLECTIVE(mlsl_allgatherv(send_buf, COUNT, recv_buf, recv_counts, mlsl_dtype_float, &coll_attr, &request),
                   "persistent_allgatherv");

    coll_attr.to_cache = 0;
    RUN_COLLECTIVE(mlsl_allgatherv(send_buf, COUNT, recv_buf, recv_counts, mlsl_dtype_float, &coll_attr, &request),
                   "regular_allgatherv");

    free(recv_counts);
    free(recv_buf);

    MLSL_CALL(mlsl_finalize());

    return 0;
}

