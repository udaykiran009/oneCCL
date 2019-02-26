#include "base.h"

/* mg sizes in bytes in backprop order */

size_t msg_sizes_test[] = { 256, 256 };

size_t msg_sizes_vgg16[] = { 16384000, 4000, 67108864, 16384, 411041792, 16384, 9437184, 2048, 9437184, 2048, 9437184, 2048,
                             9437184, 2048, 9437184, 2048, 4718592, 2048, 2359296, 1024, 2359296, 1024, 1179648, 1024, 589824,
                             512, 294912, 512, 147456, 256, 6912, 256 };

#define DELIMETER "---------------------------------------------------------------------------------------------------------------------------------------------\n"

#define PRINT_HEADER()                                      \
  do {                                                      \
      printf(DELIMETER);                                    \
      printf("msg_idx | size (bytes) | "                    \
             "msg_time (usec) | msg_start_time (usec) | "   \
             "msg_wait_time (usec) | msg_iso_time (usec) |" \
             "avg_msg_time(usec)|stddev (%%)|\n");          \
      printf(DELIMETER);                                    \
  } while (0)

#define msg_sizes msg_sizes_vgg16

size_t comp_iter_time_ms = 0;

#define sizeofa(arr)        (sizeof(arr) / sizeof(*arr))
#define DTYPE               float
#define CACHELINE_SIZE      64

#define MSG_COUNT         sizeofa(msg_sizes)
#define ITER_COUNT        20
#define WARMUP_ITER_COUNT 4

size_t min_priority, max_priority;

int collect_iso = 1;

void* msg_buffers[MSG_COUNT];
int msg_priorities[MSG_COUNT];
int msg_completions[MSG_COUNT];
mlsl_request_t msg_requests[MSG_COUNT];

double tmp_start_timer, tmp_stop_timer;
double iter_start, iter_stop, iter_timer, iter_iso_timer;

double iter_timer_avg, iter_timer_stddev;

double msg_starts[MSG_COUNT];
double msg_stops[MSG_COUNT];
double msg_timers[MSG_COUNT];
double msg_iso_timers[MSG_COUNT];

double msg_pure_start_timers[MSG_COUNT];
double msg_pure_wait_timers[MSG_COUNT];

double msg_timers_avg[MSG_COUNT];
double msg_timers_stddev[MSG_COUNT];

size_t comp_delay_ms;

size_t get_dtype_size(mlsl_datatype_t dtype)
{
    size_t dtype_size = 1;
    switch (dtype)
    {
        case mlsl_dtype_char: { dtype_size = 1; break; }
        case mlsl_dtype_int: { dtype_size = 4; break; }
        case mlsl_dtype_bfp16: { dtype_size = 2; break; }
        case mlsl_dtype_float: { dtype_size = 4; break; }
        case mlsl_dtype_double: { dtype_size = 8; break; }
        case mlsl_dtype_int64: { dtype_size = 8; break; }
        case mlsl_dtype_uint64: { dtype_size = 8; break; }
        default: ASSERT(0, "unexpected dtype %d", dtype);
    }
    return dtype_size;
}

void do_iter(size_t iter_idx)
{
    if (rank == 0)
        printf("started iter %zu\n", iter_idx); fflush(stdout);

    size_t idx, msg_idx;

    if (collect_iso)
    {
        mlsl_barrier(NULL);

        iter_start = when();
        for (idx = 0; idx < MSG_COUNT; idx++)
        {
            tmp_start_timer = when();
            MLSL_CALL(mlsl_allreduce(msg_buffers[idx], msg_buffers[idx], msg_sizes[idx] / get_dtype_size(mlsl_dtype_float),
                                     mlsl_dtype_float, mlsl_reduction_sum, &coll_attr, NULL, &msg_requests[idx]));
            MLSL_CALL(mlsl_wait(msg_requests[idx]));
            tmp_stop_timer = when();
            msg_iso_timers[idx] += (tmp_stop_timer - tmp_start_timer);
        }
        iter_stop = when();
        iter_iso_timer += (iter_stop - iter_start);
        collect_iso = 0;
    }

    mlsl_barrier(NULL);

    memset(msg_completions, 0, MSG_COUNT * sizeof(int));
    size_t completions = 0;

    iter_start = when();
    size_t priority;
    for (idx = 0; idx < MSG_COUNT; idx++)
    {
        if (idx % 2 == 0) usleep(comp_delay_ms * 1000);

        priority = min_priority + idx;
        if (priority > max_priority) priority = max_priority;
        coll_attr.priority = priority;

        msg_starts[idx] = when();
        tmp_start_timer = when();
        MLSL_CALL(mlsl_allreduce(msg_buffers[idx], msg_buffers[idx], msg_sizes[idx] / get_dtype_size(mlsl_dtype_float),
                                 mlsl_dtype_float, mlsl_reduction_sum, &coll_attr, NULL, &msg_requests[idx]));
        tmp_stop_timer = when();
        msg_pure_start_timers[idx] += (tmp_stop_timer - tmp_start_timer);
    }

    while (completions < MSG_COUNT)
    {
        for (idx = 0; idx < MSG_COUNT; idx++)
        {
            /* complete in reverse list order */
            msg_idx = (MSG_COUNT - idx - 1);

            /* complete in direct list order */
            //msg_idx = idx;

            if (msg_completions[msg_idx]) continue;

            int is_completed = 0;

            tmp_start_timer = when();

//            MLSL_CALL(mlsl_wait(msg_requests[msg_idx])); is_completed = 1;
            MLSL_CALL(mlsl_test(msg_requests[msg_idx], &is_completed));
            
            tmp_stop_timer = when();
            msg_pure_wait_timers[msg_idx] += (tmp_stop_timer - tmp_start_timer);

            if (is_completed)
            {
                msg_stops[msg_idx] = when();
                msg_timers[msg_idx] += (msg_stops[msg_idx] - msg_starts[msg_idx]);
                msg_completions[msg_idx] = 1;
                completions++;
            }
        }
    }
    iter_stop = when();
    iter_timer += (iter_stop - iter_start);

    if (rank == 0)
        printf("completed iter %zu\n", iter_idx); fflush(stdout);
}

int main()
{
    test_init();

    mlsl_get_priority_range(&min_priority, &max_priority);

    char* comp_iter_time_ms_env = getenv("COMP_ITER_TIME_MS");
    if (comp_iter_time_ms_env)
    {
        comp_iter_time_ms = atoi(comp_iter_time_ms_env);
    }
    comp_delay_ms = 2 * comp_iter_time_ms / MSG_COUNT;

    size_t total_msg_size = 0;
    size_t idx;
    for (idx = 0; idx < MSG_COUNT; idx++)
        total_msg_size += msg_sizes[idx];

    if (rank == 0)
    {
        printf("iter_count: %d, warmup_iter_count: %d\n", ITER_COUNT, WARMUP_ITER_COUNT);
        printf("msg_count: %zu, total_msg_size: %zu bytes\n", MSG_COUNT, total_msg_size);
        printf("comp_iter_time_ms: %zu, comp_delay_ms: %zu (between each pair of messages)\n", comp_iter_time_ms, comp_delay_ms);
        printf("messages are started in direct order and completed in reverse order\n");
        printf("min_priority %zu, max_priority %zu\n", min_priority, max_priority);
        fflush(stdout);
    }

    for (idx = 0; idx < MSG_COUNT; idx++)
    {
        size_t msg_size = msg_sizes[idx];
        int pm_ret = posix_memalign(&(msg_buffers[idx]), CACHELINE_SIZE, msg_size);
        assert((pm_ret == 0) && msg_buffers[idx]);
        (void)pm_ret;
        memset(msg_buffers[idx], 'a' + idx, msg_size);
        msg_priorities[idx] = idx;
    }

    /* warmup */
    collect_iso = 0;
    for (idx = 0; idx < WARMUP_ITER_COUNT; idx++)
    {
        do_iter(idx);
    }

    /* reset timers */
    iter_start = iter_stop = iter_timer = iter_iso_timer = 0;
    for (idx = 0; idx < MSG_COUNT; idx++)
    {
        msg_starts[idx] = msg_stops[idx] = msg_timers[idx] = msg_iso_timers[idx] =
            msg_pure_start_timers[idx] = msg_pure_wait_timers[idx] = 0;
    }

    /* main loop */
    collect_iso = 1;
    for (idx = 0; idx < ITER_COUNT; idx++)
    {
        do_iter(idx);
    }

    iter_timer /= ITER_COUNT;
    for (idx = 0; idx < MSG_COUNT; idx++)
    {
        msg_timers[idx] /= ITER_COUNT;
        msg_pure_start_timers[idx] /= ITER_COUNT;
        msg_pure_wait_timers[idx] /= ITER_COUNT;
    }

    mlsl_barrier(NULL);

    double* recv_msg_timers = (double*)malloc(size * MSG_COUNT * sizeof(double));
    size_t* recv_msg_timers_counts = (size_t*)malloc(size * sizeof(size_t));
    for (idx = 0; idx < size; idx++)
        recv_msg_timers_counts[idx] = MSG_COUNT;

    double* recv_iter_timers = (double*)malloc(size * sizeof(double));
    size_t* recv_iter_timers_counts = (size_t*)malloc(size * sizeof(size_t));
    for (idx = 0; idx < size; idx++)
        recv_iter_timers_counts[idx] = 1;

    mlsl_request_t timer_req = NULL;
    mlsl_coll_attr_t attr;
    memset(&attr, 0, sizeof(mlsl_coll_attr_t));
    coll_attr.to_cache = 1;

    MLSL_CALL(mlsl_allgatherv(msg_timers, MSG_COUNT, recv_msg_timers, recv_msg_timers_counts,
                              mlsl_dtype_double, &attr, NULL, &timer_req));
    MLSL_CALL(mlsl_wait(timer_req));

    MLSL_CALL(mlsl_allgatherv(&iter_timer, 1, recv_iter_timers, recv_iter_timers_counts,
                              mlsl_dtype_double, &attr, NULL, &timer_req));
    MLSL_CALL(mlsl_wait(timer_req));

    if (rank == 0)
    {
        size_t rank_idx;
        for (idx = 0; idx < MSG_COUNT; idx++)
        {
            msg_timers_avg[idx] = 0;
            for (rank_idx = 0; rank_idx < size; rank_idx++)
            {
                double val = recv_msg_timers[rank_idx * MSG_COUNT + idx];
                msg_timers_avg[idx] += val;
            }
            msg_timers_avg[idx] /= size;

            msg_timers_stddev[idx] = 0;
            double sum = 0;
            for (rank_idx = 0; rank_idx < size; rank_idx++)
            {
                double avg = msg_timers_avg[idx];
                double val = recv_msg_timers[rank_idx * MSG_COUNT + idx];
                sum += (val - avg) * (val - avg);
            }
            msg_timers_stddev[idx] = sqrt(sum / size) / msg_timers_avg[idx] * 100;
            printf("size %10zu bytes, avg %10.2lf us, stddev %5.1lf %%\n",
                    msg_sizes[idx], msg_timers_avg[idx], msg_timers_stddev[idx]);
        }

        iter_timer_avg = 0;
        for (rank_idx = 0; rank_idx < size; rank_idx++)
        {
            double val = recv_iter_timers[rank_idx];
            printf("rank %zu, time %f\n", rank_idx, val);
            iter_timer_avg += val;
        }
        iter_timer_avg /= size;

        iter_timer_stddev = 0;
        double sum = 0;
        for (rank_idx = 0; rank_idx < size; rank_idx++)
        {
            double avg = iter_timer_avg;
            double val = recv_iter_timers[rank_idx];
            sum += (val - avg) * (val - avg);
        }
        iter_timer_stddev = sqrt(sum / size) / iter_timer_avg * 100;
    }
    mlsl_barrier(NULL);

    if (rank == 0) PRINT_HEADER();

    for (idx = 0; idx < MSG_COUNT; idx++)
    {
        free(msg_buffers[idx]);
        if (rank == 0)
        {
            printf("%7zu | %12zu | %15.2lf | %21.2lf | %20.2lf | %19.2lf | %16.1lf | %10.1lf",
                   idx, msg_sizes[idx], msg_timers[idx],
                   msg_pure_start_timers[idx],
                   msg_pure_wait_timers[idx],
                   msg_iso_timers[idx],
                   msg_timers_avg[idx],
                   msg_timers_stddev[idx]);
            printf("\n");
        }
    }

    if (rank == 0) PRINT_HEADER();

    if (rank == 0)
    {
        printf("iter_time        (usec): %12.2lf\n", iter_timer);
        printf("iter_time_avg    (usec): %12.2lf\n", iter_timer_avg);
        printf("iter_time_stddev (%%): %12.2lf\n", iter_timer_stddev);
        printf("iter_iso_time    (usec): %12.2lf\n", iter_iso_timer);
    }

    test_finalize();

    free(recv_msg_timers);
    free(recv_msg_timers_counts);
    free(recv_iter_timers);
    free(recv_iter_timers_counts);

    return 0;
}
