#include "base.h"

/* VGG16 in backprop order */
size_t msg_sizes[] = { 16384000, 4000, 67108864, 16384, 411041792, 16384, 9437184, 2048, 9437184, 2048, 9437184, 2048,
                      9437184, 2048, 9437184, 2048, 4718592, 2048, 2359296, 1024, 2359296, 1024, 1179648, 1024, 589824,
                      512, 294912, 512, 147456, 256, 6912, 256 };

size_t comp_iter_time_ms = 0;

#define sizeofa(arr)        (sizeof(arr) / sizeof(*arr))
#define DTYPE               float
#define CACHELINE_SIZE      64

#define MSG_COUNT         sizeofa(msg_sizes)
#define ITER_COUNT        20
#define WARMUP_ITER_COUNT 4

size_t min_priority, max_priority;

int collect_iso = 1;

size_t total_msg_count;
void* msg_buffers[MSG_COUNT];
int msg_priorities[MSG_COUNT];
int msg_completions[MSG_COUNT];
mlsl_request_t msg_requests[MSG_COUNT];

double tmp_start_timer, tmp_stop_timer;
double iter_start, iter_stop, iter_timer, iter_iso_timer;

double msg_starts[MSG_COUNT];
double msg_stops[MSG_COUNT];
double msg_timers[MSG_COUNT];
double msg_iso_timers[MSG_COUNT];

double msg_pure_start_timers[MSG_COUNT];
double msg_pure_wait_timers[MSG_COUNT];

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
        default: assert(0);
    }
    return dtype_size;
}

void do_iter(size_t iter_idx)
{
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

        coll_attr.to_cache = 1;
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
            msg_idx = (MSG_COUNT - idx - 1);

            if (msg_completions[msg_idx]) continue;

            int is_completed = 0;

            tmp_start_timer = when();
            MLSL_CALL(mlsl_wait(msg_requests[msg_idx]));
            is_completed = 1;
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
    collect_iso = 1;

    /* main loop */
    for (idx = 0; idx < ITER_COUNT; idx++)
    {
        do_iter(idx);
    }

    if (rank == 0)
    {
        printf("-------------------------------------------------------------------------------------------------------------\n");
        printf("msg_idx | size (bytes) | msg_time (usec) | msg_start_time (usec) | msg_wait_time (usec) | msg_iso_time (usec)\n");
        printf("-------------------------------------------------------------------------------------------------------------\n");
    }

    for (idx = 0; idx < MSG_COUNT; idx++)
    {
        free(msg_buffers[idx]);
        if (rank == 0)
        {
            printf("%7zu | %12zu | %15.2lf | %21.2lf | %20.2lf | %19.2lf ",
                   idx, msg_sizes[idx], msg_timers[idx] / ITER_COUNT,
                   msg_pure_start_timers[idx] / ITER_COUNT,
                   msg_pure_wait_timers[idx] / ITER_COUNT,
                   msg_iso_timers[idx] /*/ ITER_COUNT*/);
            printf("\n");
        }
    }

    if (rank == 0)
    {
        printf("-------------------------------------------------------------------------------------------------------------\n");
        printf("iter_time     (usec): %12.2lf\n", iter_timer / ITER_COUNT);
        printf("iter_iso_time (usec): %12.2lf\n", iter_iso_timer /*/ ITER_COUNT*/);
    }

    test_finalize();

    return 0;
}
