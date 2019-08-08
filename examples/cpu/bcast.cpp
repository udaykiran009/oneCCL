#include "base.h"

#define RUN_COLLECTIVE(start_cmd, name)                                    \
  do {                                                                     \
      t = 0;                                                               \
      for (iter_idx = 0; iter_idx < ITERS; iter_idx++)                     \
      {                                                                    \
          for (idx = 0; idx < COUNT; idx++)                                \
          {                                                                \
              if (rank == COLL_ROOT) buf[idx] = idx;                       \
              else buf[idx] = 0.0;                                         \
          }                                                                \
          t1 = when();                                                     \
          CCL_CALL(start_cmd);                                             \
          CCL_CALL(ccl_wait(request));                                     \
          t2 = when();                                                     \
          t += (t2 - t1);                                                  \
      }                                                                    \
      ccl_barrier(NULL, NULL);                                             \
      for (idx = 0; idx < COUNT; idx++)                                    \
      {                                                                    \
          if (buf[idx] != idx)                                             \
          {                                                                \
              printf("iter %zu, idx %zu, expected %zu, got %f\n",          \
                      iter_idx, idx, idx, buf[idx]);                       \
              ASSERT(0, "unexpected value");                               \
          }                                                                \
      }                                                                    \
      printf("[%zu] avg %s time: %8.2lf us\n", rank, name, t / ITERS);     \
      fflush(stdout);                                                      \
  } while (0)

int main()
{
    float buf[COUNT];

    test_init();

    coll_attr.to_cache = 1;
    RUN_COLLECTIVE(ccl_bcast(buf, COUNT, ccl_dtype_float, COLL_ROOT, &coll_attr, NULL, NULL, &request),
                   "persistent_bcast");

    coll_attr.to_cache = 0;
    RUN_COLLECTIVE(ccl_bcast(buf, COUNT, ccl_dtype_float, COLL_ROOT, &coll_attr, NULL, NULL, &request),
                   "regular_bcast");

    test_finalize();

    if (rank == 0)
        printf("PASSED\n");

    return 0;
}
