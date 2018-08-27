#include "exec.h"
#include "global.h"
#include "sched_cache.h"

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

        if (env_data.worker_offload)
        {
            mlsl_worker_start(e->workers[idx]);
            mlsl_worker_pin(e->workers[idx], env_data.worker_affinity[idx]);
            MLSL_LOG(INFO, "started worker # %zu", idx);
        }
    }

    *executor = e;

    return mlsl_status_success;
}

mlsl_status_t mlsl_executor_free(mlsl_executor *executor)
{
    size_t idx;
    for (idx = 0; idx < executor->worker_count; idx++)
    {
        if (env_data.worker_offload)
        {
            mlsl_worker_stop(executor->workers[idx]);
            MLSL_LOG(INFO, "stopped worker # %zu", idx);
        }

        mlsl_worker_free(executor->workers[idx]);
        mlsl_sched_queue_free(executor->workers[idx]->sched_queue);
    }

    atl_finalize(executor->atl_desc, executor->atl_comms);
    MLSL_FREE(executor->workers);
    MLSL_FREE(executor);

    return mlsl_status_success;
}

mlsl_status_t mlsl_executor_start(mlsl_executor *executor, mlsl_sched *sched)
{
    // TODO:
    // optimize sched if possible

    // TODO: offload sched clone/adjust/reset on worker side

    /* find sched in cache */
    size_t idx;
    mlsl_sched_cache_entry *e = sched->cache_entry;
    MLSL_ASSERT(e && (e->origin_sched == sched) && e->worker_scheds);

    mlsl_sched **worker_scheds = e->worker_scheds;

    for (idx = 0; idx < e->worker_sched_count; idx++)
    {
        worker_scheds[idx]->first_progress = 1;
        mlsl_sched_adjust_tag(worker_scheds[idx]);
    }

    if (executor->proc_idx == 0)
        mlsl_sched_dump(e->origin_sched, "origin_sched");

    /* add scheds into worker queues */
    sched->req->completion_counter = e->worker_sched_count;
    for (idx = 0; idx < e->worker_sched_count; idx++)
    {
        if (executor->proc_idx == 0)
            mlsl_sched_dump(worker_scheds[idx], "worker_sched");

        mlsl_sched_queue *queue = executor->workers[idx % executor->worker_count]->sched_queue;
        size_t priority = 0;
        if (sched->coll_desc->ctype == mlsl_coll_barrier)
            mlsl_sched_queue_get_max_priority(queue, &priority);

        worker_scheds[idx]->req = sched->req;
        mlsl_sched_queue_add(queue, worker_scheds[idx], priority);
    }

    return mlsl_status_success;
}

mlsl_status_t mlsl_executor_wait(mlsl_executor *executor, mlsl_request *req)
{
    MLSL_LOG(DEBUG, "req %p, req->cc %d", req, req->completion_counter);

    while (__atomic_load_n(&req->completion_counter, __ATOMIC_ACQUIRE))
    {
        if (env_data.worker_offload)
            _mm_pause();
        else
        {
            size_t idx, processed_count;
            for (idx = 0; idx < executor->worker_count; idx++)
                mlsl_worker_peek_and_progress(executor->workers[idx], &processed_count);
        }
    }

    return mlsl_status_success;
}

mlsl_status_t mlsl_executor_test(mlsl_executor *executor, mlsl_request *req, int *is_completed)
{
    MLSL_LOG(DEBUG, "req %p, req->cc %d", req, req->completion_counter);

    size_t completion_counter = __atomic_load_n(&req->completion_counter, __ATOMIC_ACQUIRE);
    if (completion_counter)
    {
        if (env_data.worker_offload)
            _mm_pause();
        else
        {
            size_t idx, processed_count;
            for (idx = 0; idx < executor->worker_count; idx++)
                mlsl_worker_peek_and_progress(executor->workers[idx], &processed_count);
        }
        *is_completed = (!__atomic_load_n(&req->completion_counter, __ATOMIC_ACQUIRE));
    }
    else
        *is_completed = 1;

    return mlsl_status_success;
}
