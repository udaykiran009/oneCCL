#include "base.h"

#define BUF_COUNT         (1024)
#define ELEM_COUNT        (1024 * 1024 / 128)
#define SINGLE_ELEM_COUNT (BUF_COUNT * ELEM_COUNT)
#define ALIGNMENT         (2 * 1024 * 1024)
#define DTYPE             float
#define CCL_DTYPE         (ccl_dtype_float)
#define MATCH_ID_SIZE     (64)

void fill_buffers(void** send_bufs, void** recv_bufs, size_t buf_count, size_t elem_count)
{
    size_t b_idx, e_idx;
    for (b_idx = 0; b_idx < buf_count; b_idx++)
    {
        for (e_idx = 0; e_idx < elem_count; e_idx++)
        {
            ((DTYPE*)send_bufs[b_idx])[e_idx] = rank;
            ((DTYPE*)recv_bufs[b_idx])[e_idx] = 0;
        }
    }
}

void check_buffers(void** send_bufs, void** recv_bufs, size_t buf_count, size_t elem_count)
{
    size_t b_idx, e_idx;
    DTYPE sbuf_expected = rank;
    DTYPE rbuf_expected = (size - 1) * ((float)size / 2);
    DTYPE value;
    for (b_idx = 0; b_idx < buf_count; b_idx++)
    {
        for (e_idx = 0; e_idx < elem_count; e_idx++)
        {
            value = ((DTYPE*)send_bufs[b_idx])[e_idx];
            if (value != sbuf_expected)
            {
                printf("send_bufs: buf_idx %zu, elem_idx %zu, expected %f, got %f\n",
                       b_idx, e_idx, sbuf_expected, value);
                ASSERT(0, "unexpected value"); 
            }

            value = ((DTYPE*)recv_bufs[b_idx])[e_idx];
            if (value != rbuf_expected)
            {
                printf("recv_bufs: buf_idx %zu, elem_idx %zu, expected %f, got %f\n",
                       b_idx, e_idx, rbuf_expected, value);
                ASSERT(0, "unexpected value"); 
            }
        }
    }
}

void print_timings(double* timer, size_t elem_count, size_t buf_count)
{
    double* timers = (double*)malloc(size * sizeof(double));
    size_t* recv_counts = (size_t*)malloc(size * sizeof(size_t));

    size_t idx;
    for (idx = 0; idx < size; idx++)
        recv_counts[idx] = 1;

    ccl_request_t request = NULL;
    ccl_coll_attr_t attr;
    memset(&attr, 0, sizeof(ccl_coll_attr_t));
    CCL_CALL(ccl_allgatherv(timer, 1, timers, recv_counts, ccl_dtype_double, &attr, NULL, NULL, &request));
    CCL_CALL(ccl_wait(request));

    if (rank == 0)
    {
        double avg_timer = 0;
        double avg_timer_per_buf = 0;
        for (idx = 0; idx < size; idx++)
        {
            avg_timer += timers[idx];
        }
        avg_timer /= (ITERS * size);
        avg_timer_per_buf = avg_timer / buf_count;

        double stddev_timer = 0;
        double sum = 0;
        for (idx = 0; idx < size; idx++)
        {
            double val = timers[idx] / ITERS;
            sum += (val - avg_timer) * (val - avg_timer);
        }
        stddev_timer = sqrt(sum / size) / avg_timer * 100;
        printf("size %10zu x %5zu bytes, avg %10.2lf us, avg_per_buf %10.2f, stddev %5.1lf %%\n",
                elem_count * sizeof(DTYPE), buf_count, avg_timer, avg_timer_per_buf, stddev_timer);
    }
    ccl_barrier(NULL, NULL);
    free(timers);
    free(recv_counts);
}

int main()
{
    size_t idx, iter_idx, count;
    DTYPE* send_bufs[BUF_COUNT];
    DTYPE* recv_bufs[BUF_COUNT];
    DTYPE* single_send_buf = NULL;
    DTYPE* single_recv_buf = NULL;
    ccl_request_t reqs[BUF_COUNT];
    double t, t1, t2;
    int check_values = 1;

    test_init();

    if (rank == 0)
        printf("buf_count %d, ranks %zu, check_values %d\n", BUF_COUNT, size, check_values);

    int result = 0;
    for (idx = 0; idx < BUF_COUNT; idx++)
    {
        result = posix_memalign((void**)&send_bufs[idx], ALIGNMENT, ELEM_COUNT * sizeof(DTYPE));
        result = posix_memalign((void**)&recv_bufs[idx], ALIGNMENT, ELEM_COUNT * sizeof(DTYPE));
    }
    result = posix_memalign((void**)&single_send_buf, ALIGNMENT, SINGLE_ELEM_COUNT * sizeof(DTYPE));
    result = posix_memalign((void**)&single_recv_buf, ALIGNMENT, SINGLE_ELEM_COUNT * sizeof(DTYPE));
    (void)result;

    /* warmup */
    coll_attr.to_cache = 0;
    for (count = 1; count <= ELEM_COUNT; count *= 2)
    {
        for (idx = 0; idx < BUF_COUNT; idx++)
        {
            ccl_allreduce(send_bufs[idx], recv_bufs[idx], count, CCL_DTYPE,
                          ccl_reduction_sum, &coll_attr, NULL, NULL, &reqs[idx]);
        }
        for (idx = 0; idx < BUF_COUNT; idx++)
        {
            ccl_wait(reqs[idx]);
        }
    }

    ccl_barrier(NULL, NULL);

    char match_id[MATCH_ID_SIZE];
    coll_attr.to_cache = 1;
    coll_attr.match_id = match_id;

    for (count = 1; count <= ELEM_COUNT; count *= 2)
    {
        t = 0;
        for (iter_idx = 0; iter_idx < ITERS; iter_idx++)
        {
            if (check_values) fill_buffers((void**)send_bufs, (void**)recv_bufs, BUF_COUNT, count);
            t1 = when();
            for (idx = 0; idx < BUF_COUNT; idx++)
            {
                snprintf(match_id, sizeof(match_id), "count_%zu_idx_%zu", count, idx);
                ccl_allreduce(send_bufs[idx], recv_bufs[idx], count, CCL_DTYPE,
                               ccl_reduction_sum, &coll_attr, NULL, NULL, &reqs[idx]);
            }
            for (idx = 0; idx < BUF_COUNT; idx++)
            {
                ccl_wait(reqs[idx]);
            }
            t2 = when();
            t += (t2 - t1);
            if (check_values) check_buffers((void**)send_bufs, (void**)recv_bufs, BUF_COUNT, count);
        }
        print_timings(&t, count, BUF_COUNT);
    }

    ccl_barrier(NULL, NULL);

    coll_attr.to_cache = 1;
    for (count = BUF_COUNT; count <= SINGLE_ELEM_COUNT; count *= 2)
    {
        t = 0;
        for (iter_idx = 0; iter_idx < ITERS; iter_idx++)
        {
            t1 = when();
            snprintf(match_id, sizeof(match_id), "single_count_%zu", count);
            ccl_allreduce(single_send_buf, single_recv_buf, count, CCL_DTYPE,
                           ccl_reduction_sum, &coll_attr, NULL, NULL, &reqs[0]);
            ccl_wait(reqs[0]);
            t2 = when();
            t += (t2 - t1);
        }
        print_timings(&t, count, 1);
    }

    for (idx = 0; idx < BUF_COUNT; idx++)
    {
        free(send_bufs[idx]);
        free(recv_bufs[idx]);
    }
    free(single_send_buf);
    free(single_recv_buf);

    test_finalize();

    return 0;
}
