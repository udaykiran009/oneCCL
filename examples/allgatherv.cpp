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
          ICCL_CALL(start_cmd);                                            \
          ICCL_CALL(iccl_wait(request));                                   \
          t2 = when();                                                     \
          t += (t2 - t1);                                                  \
      }                                                                    \
      iccl_barrier(NULL);                                                  \
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
    float *recv_buf;
    size_t *recv_counts;

    test_init();

    recv_buf = static_cast<float*>(malloc(size * COUNT * sizeof(float)));
    recv_counts = static_cast<size_t*>(malloc(size * sizeof(size_t)));

    for (idx = 0; idx < size; idx++)
        recv_counts[idx] = COUNT;

    coll_attr.to_cache = 1;
    RUN_COLLECTIVE(iccl_allgatherv(send_buf, COUNT, recv_buf, recv_counts, iccl_dtype_float, &coll_attr, NULL, &request),
                   "persistent_allgatherv");

    coll_attr.to_cache = 0;
    RUN_COLLECTIVE(iccl_allgatherv(send_buf, COUNT, recv_buf, recv_counts, iccl_dtype_float, &coll_attr, NULL, &request),
                   "regular_allgatherv");

    free(recv_counts);
    free(recv_buf);

    test_finalize();

    return 0;
}

