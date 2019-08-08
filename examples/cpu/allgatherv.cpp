#include "base.h"

#define RUN_COLLECTIVE(start_cmd, name)                                    \
  do {                                                                     \
      t = 0;                                                               \
      float expected = 1.0;                                                \
      for (iter_idx = 0; iter_idx < ITERS; iter_idx++)                     \
      {                                                                    \
          for (idx = 0; idx < COUNT; idx++)                                \
              send_buf[idx] = expected;                                    \
          for (idx = 0; idx < size * COUNT; idx++)                         \
              recv_buf[idx] = 0.0;                                         \
          t1 = when();                                                     \
          CCL_CALL(start_cmd);                                             \
          CCL_CALL(ccl_wait(request));                                     \
          t2 = when();                                                     \
          t += (t2 - t1);                                                  \
      }                                                                    \
      ccl_barrier(NULL, NULL);                                             \
      for (idx = 0; idx < size * COUNT; idx++)                             \
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
    float* recv_buf;
    size_t* recv_counts;

    test_init();

    recv_buf = static_cast<float*>(malloc(size * COUNT * sizeof(float)));
    recv_counts = static_cast<size_t*>(malloc(size * sizeof(size_t)));

    for (idx = 0; idx < size; idx++)
        recv_counts[idx] = COUNT;

    coll_attr.to_cache = 1;
    RUN_COLLECTIVE(ccl_allgatherv(send_buf, COUNT, recv_buf, recv_counts, ccl_dtype_float, &coll_attr, NULL, NULL, &request),
                   "persistent_allgatherv");

    coll_attr.to_cache = 0;
    RUN_COLLECTIVE(ccl_allgatherv(send_buf, COUNT, recv_buf, recv_counts, ccl_dtype_float, &coll_attr, NULL, NULL, &request),
                   "regular_allgatherv");

    free(recv_counts);
    free(recv_buf);

    test_finalize();

    if (rank == 0)
        printf("PASSED\n");

    return 0;
}

