#include "exec.h"
#include "global.h"
#include "log.h"
#include "mlsl.h"
#include "sched_cache.h"

mlsl_status_t mlsl_init()
{
    mlsl_env_parse();
    mlsl_env_print();
    mlsl_sched_cache_create(&global_sched_cache);
    mlsl_parallelizer_create(env_data.worker_count, &global_parallelizer);
    mlsl_executor_create(env_data.worker_count, 1 /* priority_count */, &global_executor);
    mlsl_comm_create(global_executor->proc_idx, global_executor->proc_count, &global_comm);

    global_data.sched_cache = global_sched_cache;
    global_data.parallelizer = global_parallelizer;
    global_data.executor = global_executor;
    global_data.comm = global_comm;

    return mlsl_status_success;
}

mlsl_status_t mlsl_finalize()
{
    mlsl_comm_free(global_data.comm);
    mlsl_executor_free(global_data.executor);
    mlsl_parallelizer_free(global_data.parallelizer);
    mlsl_sched_cache_free(global_data.sched_cache);
    mlsl_env_free();
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
    mlsl_executor_wait(global_data.executor, req);

    mlsl_sched *sched = req->sched;
    MLSL_ASSERT(sched);

    if (sched->type == mlsl_sched_non_persistent)
        mlsl_sched_free(sched);

    return mlsl_status_success;
}

mlsl_status_t MLSL_API mlsl_test(mlsl_request *req, int *is_completed)
{
    mlsl_executor_test(global_data.executor, req, is_completed);

    if (*is_completed)
    {
        mlsl_sched *sched = req->sched;
        MLSL_ASSERTP(sched);

        if (sched->type == mlsl_sched_non_persistent)
            mlsl_sched_free(sched);
    }

    return mlsl_status_success;
}
