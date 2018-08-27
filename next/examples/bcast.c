#include "base.h"

#define RUN_COLLECTIVE(start_cmd, name)                                    \
  do {                                                                     \
      t = 0;                                                               \
      float expected = 1.0;                                                \
      for (iter_idx = 0; iter_idx < ITERS; iter_idx++)                     \
      {                                                                    \
          for (idx = 0; idx < COUNT; idx++)                                \
          {                                                                \
              if (proc_idx == ROOT) buf[idx] = expected;                   \
              else buf[idx] = 0.0;                                         \
          }                                                                \
          t1 = when();                                                     \
          MLSL_CALL(start_cmd);                                            \
          MLSL_CALL(mlsl_wait(request));                                   \
          t2 = when();                                                     \
          t += (t2 - t1);                                                  \
      }                                                                    \
      mlsl_barrier();                                                      \
      for (idx = 0; idx < COUNT; idx++)                                    \
      {                                                                    \
          if (buf[idx] != expected)                                        \
          {                                                                \
              printf("iter %zu, idx %zu, expected %f, got %f\n",           \
                      iter_idx, idx, expected, buf[idx]);                  \
              assert(0);                                                   \
          }                                                                \
      }                                                                    \
      printf("[%zu] avg %s time: %8.2lf us\n", proc_idx, name, t / ITERS); \
      fflush(stdout);                                                      \
  } while (0)

int main()
{
    mlsl_status_t status = mlsl_status_success;
    float buf[COUNT];

    MLSL_CALL(mlsl_init());

    proc_idx = mlsl_get_proc_idx();
    proc_count = mlsl_get_proc_count();

    MLSL_CALL(mlsl_sched_bcast(buf, COUNT, mlsl_dtype_float, ROOT, &sched));
    MLSL_CALL(mlsl_sched_commit(sched));
    RUN_COLLECTIVE(mlsl_sched_start(sched, &request), "persistent_bcast");
    MLSL_CALL(mlsl_sched_free(sched));

    RUN_COLLECTIVE(mlsl_bcast(buf, COUNT, mlsl_dtype_float, ROOT, &request),
                   "regular_bcast");

    MLSL_CALL(mlsl_finalize());

    return 0;
}
