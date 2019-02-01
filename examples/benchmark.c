#include "base.h"

#define BUF_COUNT  (16)
#define ELEM_COUNT (2 * 1024 * 1024)
#define ALIGNMENT  (2 * 1024 * 1024)
#define DTYPE      float
#define MLSL_DTYPE (mlsl_dtype_float)

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

void print_timings(double* timer, size_t count)
{
    double* timers = (double*)malloc(size * sizeof(double));
    size_t* recv_counts = (size_t*)malloc(size * sizeof(size_t));

    size_t idx;
    for (idx = 0; idx < size; idx++)
        recv_counts[idx] = 1;

    mlsl_request_t request = NULL;
    mlsl_coll_attr_t attr;
    memset(&attr, 0, sizeof(mlsl_coll_attr_t));
    MLSL_CALL(mlsl_allgatherv(timer, 1, timers, recv_counts, mlsl_dtype_double, &attr, NULL, &request));
    MLSL_CALL(mlsl_wait(request));

    if (rank == 0)
    {
        double avg_timer = 0;
        for (idx = 0; idx < size; idx++)
        {
            double val = timers[idx] / (ITERS * BUF_COUNT);
            avg_timer += val;
        }
        avg_timer /= size;
        double stddev_timer = 0;
        double sum = 0;
        for (idx = 0; idx < size; idx++)
        {
            double val = timers[idx] / (ITERS * BUF_COUNT);
            sum += (val - avg_timer) * (val - avg_timer);
        }
        stddev_timer = sqrt(sum / size) / avg_timer * 100;
        printf("size %10zu bytes, avg %10.2lf us, stddev %5.1lf %%\n",
                count * sizeof(DTYPE), avg_timer, stddev_timer);
    }
    mlsl_barrier(NULL);
    free(timers);
    free(recv_counts);
}

int main()
{
    size_t idx, iter_idx, count;
    DTYPE* send_bufs[BUF_COUNT];
    DTYPE* recv_bufs[BUF_COUNT];
    mlsl_request_t reqs[BUF_COUNT];
    double t, t1, t2;
    int check_values = 0;

    test_init();

    if (rank == 0)
        printf("buf_count %d, ranks %zu, check_values %d\n", BUF_COUNT, size, check_values);

    for (idx = 0; idx < BUF_COUNT; idx++)
    {
        posix_memalign((void**)&send_bufs[idx], ALIGNMENT, ELEM_COUNT * sizeof(DTYPE));
        posix_memalign((void**)&recv_bufs[idx], ALIGNMENT, ELEM_COUNT * sizeof(DTYPE));
    }

    /* warmup */
    coll_attr.to_cache = 0;
    for (count = 1; count <= ELEM_COUNT; count *= 2)
    {
        for (idx = 0; idx < BUF_COUNT; idx++)
        {
            mlsl_allreduce(send_bufs[idx], recv_bufs[idx], count, MLSL_DTYPE,
                           mlsl_reduction_sum, &coll_attr, NULL, &reqs[idx]);
        }
        for (idx = 0; idx < BUF_COUNT; idx++)
        {
            mlsl_wait(reqs[idx]);
        }
    }

    mlsl_barrier(NULL);

    coll_attr.to_cache = 1;
    for (count = 1; count <= ELEM_COUNT; count *= 2)
    {
        t = 0;
        for (iter_idx = 0; iter_idx < ITERS; iter_idx++)
        {
            if (check_values) fill_buffers((void**)send_bufs, (void**)recv_bufs, BUF_COUNT, count);
            t1 = when();
            for (idx = 0; idx < BUF_COUNT; idx++)
            {
                mlsl_allreduce(send_bufs[idx], recv_bufs[idx], count, MLSL_DTYPE,
                               mlsl_reduction_sum, &coll_attr, NULL, &reqs[idx]);    
            }
            for (idx = 0; idx < BUF_COUNT; idx++)
            {
                mlsl_wait(reqs[idx]);
            }
            t2 = when();
            t += (t2 - t1);
            if (check_values) check_buffers((void**)send_bufs, (void**)recv_bufs, BUF_COUNT, count);
        }
        print_timings(&t, count);
    }

    for (idx = 0; idx < BUF_COUNT; idx++)
    {
        free(send_bufs[idx]);
        free(recv_bufs[idx]);
    }

    test_finalize();

    return 0;
}
