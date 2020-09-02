#include "base.h"

#define RUN_COLLECTIVE(start_cmd, name) \
    do { \
        t = 0; \
        for (iter_idx = 0; iter_idx < ITERS; iter_idx++) { \
            for (idx = 0; idx < size * COUNT; idx++) { \
                send_buf[idx] = (int)rank; \
                recv_buf[idx] = 0; \
            } \
            t1 = when(); \
            CCL_CALL(start_cmd); \
            CCL_CALL(ccl_wait(request)); \
            t2 = when(); \
            t += (t2 - t1); \
        } \
        ccl_barrier(NULL, NULL); \
        int expected; \
        for (idx = 0; idx < size * COUNT; idx++) { \
            expected = idx / COUNT; \
            if (recv_buf[idx] != expected) { \
                printf("iter %zu, idx %zu, expected %d, got %d\n", \
                       iter_idx, \
                       idx, \
                       expected, \
                       recv_buf[idx]); \
                ASSERT(0, "unexpected value"); \
            } \
        } \
        printf("[%zu] avg %s time: %8.2lf us\n", rank, name, t / ITERS); \
        fflush(stdout); \
    } while (0)

int main() {
    int* send_buf;
    int* recv_buf;

    test_init();

    send_buf = malloc(sizeof(int) * COUNT * size);
    recv_buf = malloc(sizeof(int) * COUNT * size);

    coll_attr.to_cache = 0;
    RUN_COLLECTIVE(
        ccl_alltoall(send_buf, recv_buf, COUNT, ccl_dtype_int, &coll_attr, NULL, NULL, &request),
        "warmup_alltoall");

    coll_attr.to_cache = 1;
    RUN_COLLECTIVE(
        ccl_alltoall(send_buf, recv_buf, COUNT, ccl_dtype_int, &coll_attr, NULL, NULL, &request),
        "persistent_alltoall");

    coll_attr.to_cache = 0;
    RUN_COLLECTIVE(
        ccl_alltoall(send_buf, recv_buf, COUNT, ccl_dtype_int, &coll_attr, NULL, NULL, &request),
        "regular_alltoall");

    test_finalize();

    free(send_buf);
    free(recv_buf);

    if (rank == 0)
        printf("PASSED\n");

    return 0;
}
