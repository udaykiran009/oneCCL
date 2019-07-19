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
          ICCL_CALL(start_cmd);                                            \
          ICCL_CALL(iccl_wait(request));                                   \
          t2 = when();                                                     \
          t += (t2 - t1);                                                  \
      }                                                                    \
      iccl_barrier(NULL);                                                  \
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
    RUN_COLLECTIVE(iccl_bcast(buf, COUNT, iccl_dtype_float, COLL_ROOT, &coll_attr, NULL, &request),
                   "persistent_bcast");

    coll_attr.to_cache = 0;
    RUN_COLLECTIVE(iccl_bcast(buf, COUNT, iccl_dtype_float, COLL_ROOT, &coll_attr, NULL, &request),
                   "regular_bcast");

    test_finalize();

    return 0;
}
