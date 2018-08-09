#include "exec.h"
#include <immintrin.h>

mlsl_executor *global_executor = NULL;

mlsl_status_t mlsl_executor_create(size_t worker_count, size_t priority_count, mlsl_executor **executor)
{
    MLSL_LOG(DEBUG, "worker_count %zu, priority_count %zu",
             worker_count, priority_count);

    mlsl_executor *e = MLSL_CALLOC(sizeof(mlsl_executor), "executor");
    e->worker_count = worker_count;
    e->workers = MLSL_CALLOC(sizeof(mlsl_worker*) * worker_count, "executor->workers");

    atl_attr_t attr = { .comm_count = (worker_count * priority_count) };
    atl_status_t atl_status = atl_init(NULL, NULL, &e->proc_idx, &e->proc_count, &attr, &e->atl_comms, &e->atl_desc);
    MLSL_ASSERTP(atl_status == atl_status_success);
    MLSL_ASSERTP(e->atl_desc);
    MLSL_ASSERTP(e->atl_comms);

    MLSL_LOG(DEBUG, "proc_idx %zu, proc_count %zu, atl_desc %p",
             e->proc_idx, e->proc_count, e->atl_desc);

    size_t idx;
    for (idx = 0; idx < worker_count; idx++)
    {
        mlsl_sched_queue *queue;
        mlsl_sched_queue_create(priority_count, e->atl_comms + idx * priority_count, &queue);
        mlsl_worker_create(e, idx, queue, &(e->workers[idx]));
        mlsl_worker_start(e->workers[idx]);
        mlsl_worker_pin(e->workers[idx], env_data.worker_affinity[idx]);
        MLSL_LOG(INFO, "created worker # %zu", idx);
    }

    *executor = e;

    return mlsl_status_success;
}

mlsl_status_t mlsl_executor_free(mlsl_executor *executor)
{
    size_t idx;
    for (idx = 0; idx < executor->worker_count; idx++)
    {
        mlsl_worker_stop(executor->workers[idx]);
        mlsl_worker_free(executor->workers[idx]);
        mlsl_sched_queue_free(executor->workers[idx]->sched_queue);
        MLSL_LOG(INFO, "freed worker # %zu", idx);
    }

    atl_finalize(executor->atl_desc, executor->atl_comms);
    MLSL_FREE(executor->workers);
    MLSL_FREE(executor);

    return mlsl_status_success;
}

mlsl_status_t mlsl_executor_start(mlsl_executor *executor, mlsl_sched *sched, mlsl_request **req)
{
    // TODO:
    // optimize sched if possible
    // parallelize sched
    // cache sched

    // offload to workers
    // size_t idx;
    // for (idx = 0; idx < executor->worker_count; idx++)
    // {
    //     mlsl_sched_queue_add(executor->workers[idx]->sched_queue, &(scheds[idx]), 0 /* priority */);
    // }

    // set sched->req completion_counter according to parallelization scheme

    sched->req->completion_counter = 1;
    mlsl_sched_queue_add(executor->workers[0]->sched_queue, sched, 0 /* priority */);

    return mlsl_status_success;
}

mlsl_status_t mlsl_executor_wait(mlsl_executor *executor, mlsl_request *req)
{
    MLSL_LOG(DEBUG, "req %p, req->cc %d", req, req->completion_counter);

    // TODO: do atomic load because cc is updated from worker threads
    while (req->completion_counter)
    {
        _mm_pause();
    }
    return mlsl_status_success;
}

size_t mlsl_executor_get_proc_idx(mlsl_executor *executor)
{
    return executor->proc_idx;
}

size_t mlsl_executor_get_proc_count(mlsl_executor *executor)
{
    return executor->proc_count;
}
