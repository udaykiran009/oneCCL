#include "exec.h"
#include "global.h"
#include "log.h"
#include "mlsl.h"
#include "sched_cache.h"
#include "sched_queue.h"

mlsl_status_t mlsl_init()
{
    mlsl_env_parse();
    mlsl_env_print();
    mlsl_sched_cache_create(&global_sched_cache);
    mlsl_parallelizer_create(env_data.worker_count, &global_parallelizer);
    size_t min_priority, max_priority;
    mlsl_get_priority_range(&min_priority, &max_priority);
    mlsl_executor_create(env_data.worker_count, (max_priority - min_priority + 1), &global_executor);
    mlsl_comm_create(global_executor->proc_idx, global_executor->proc_count, &global_comm);
    mlsl_coll_create_attr(&default_coll_attr);
    default_coll_attr->to_cache = 1;

    global_data.sched_cache = global_sched_cache;
    global_data.parallelizer = global_parallelizer;
    global_data.executor = global_executor;
    global_data.comm = global_comm;
    global_data.default_coll_attr = default_coll_attr;

    return mlsl_status_success;
}

mlsl_status_t mlsl_finalize()
{
    mlsl_comm_free(global_data.comm);
    mlsl_executor_free(global_data.executor);
    mlsl_parallelizer_free(global_data.parallelizer);
    mlsl_sched_cache_free(global_data.sched_cache);
    mlsl_env_free();
    mlsl_coll_free_attr(global_data.default_coll_attr);
    return mlsl_status_success;
}

size_t mlsl_get_proc_idx()
{
    return global_data.comm->proc_idx;
}

size_t mlsl_get_proc_count()
{
    return global_data.comm->proc_count;
}

size_t mlsl_get_dtype_size(mlsl_data_type_t dtype)
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
        default: MLSL_ASSERT_FMT(0, "unexpected dtype %d", dtype);
    }
    return dtype_size;
}

mlsl_status_t mlsl_wait(mlsl_request *req)
{
    if (!req)
        return mlsl_status_success;

    mlsl_executor_wait(global_data.executor, req);

    mlsl_sched *sched = req->sched;
    MLSL_ASSERT(sched);

    if (!sched->coll_attr.to_cache)
        mlsl_sched_free(sched);

    return mlsl_status_success;
}

mlsl_status_t MLSL_API mlsl_test(mlsl_request *req, int *is_completed)
{
    if (!req)
    {
        if (is_completed) *is_completed = 1;
        return mlsl_status_success;
    }

    mlsl_executor_test(global_data.executor, req, is_completed);

    if (*is_completed)
    {
        mlsl_sched *sched = req->sched;
        MLSL_ASSERTP(sched);

        if (!sched->coll_attr.to_cache)
            mlsl_sched_free(sched);
    }

    return mlsl_status_success;
}

mlsl_status_t MLSL_API mlsl_get_priority_range(size_t *min_priority, size_t *max_priority)
{
    if (min_priority) *min_priority = 0;
    if (max_priority) *max_priority = (MLSL_SCHED_QUEUE_MAX_BINS - 1);
    if (min_priority && max_priority)
        MLSL_ASSERTP(*min_priority <= *max_priority);
    return mlsl_status_success;
}
