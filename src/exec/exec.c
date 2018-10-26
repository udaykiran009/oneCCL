#include "exec/exec.h"
#include "common/global/global.h"
#include "sched/sched_queue.h"

#include <immintrin.h>

#define MLSL_ATL_MAX_COMMS (8)

mlsl_executor *global_executor = NULL;

mlsl_status_t mlsl_executor_create(size_t worker_count, size_t priority_count, mlsl_executor **executor)
{
    MLSL_LOG(INFO, "worker_count %zu, priority_count %zu",
             worker_count, priority_count);

    mlsl_executor *e = MLSL_CALLOC(sizeof(mlsl_executor), "executor");
    e->worker_count = worker_count;
    e->workers = MLSL_CALLOC(sizeof(mlsl_worker*) * worker_count, "executor->workers");

    /* it is unlikely that ATL will provide comms for each (worker, priority) pair
       so request some moderate amount of comms and spread them evenly with repetitions */
    size_t total_comm_count = worker_count * priority_count;
    size_t comm_count = MIN(total_comm_count, MLSL_ATL_MAX_COMMS);

    if (env_data.priority_mode == mlsl_priority_none)
        comm_count = worker_count;

    MLSL_ASSERTP(comm_count >= worker_count);
    atl_attr_t attr = { .comm_count = comm_count };
    atl_status_t atl_status = atl_init(NULL, NULL, &e->proc_idx, &e->proc_count, &attr, &e->atl_comms, &e->atl_desc);
    MLSL_ASSERTP(atl_status == atl_status_success);
    MLSL_ASSERTP(e->atl_desc);
    MLSL_ASSERTP(e->atl_comms);

    MLSL_LOG(INFO, "proc_idx %zu, proc_count %zu, atl_desc %p",
             e->proc_idx, e->proc_count, e->atl_desc);

    atl_comm_t **comms = MLSL_CALLOC(sizeof(atl_comm_t *) * total_comm_count, "comms");
    size_t comms_per_worker = comm_count / worker_count;
    size_t priorities_per_comm = priority_count / comms_per_worker;
    size_t idx, pr_idx;
    for (idx = 0; idx < worker_count; idx++)
    {
        for (pr_idx = 0; pr_idx < priority_count; pr_idx++)
        {
            comms[idx * priority_count + pr_idx] =
                e->atl_comms[idx * comms_per_worker + pr_idx / priorities_per_comm];

            if (e->proc_idx == 0)
                MLSL_LOG(INFO, "map atl comms: w_idx %zu, pr_idx %zu, comm_idx %zu, comms_per_worker %zu",
                    idx, pr_idx, idx * comms_per_worker + pr_idx / priorities_per_comm, comms_per_worker);
        }
    }

    for (idx = 0; idx < worker_count; idx++)
    {
        mlsl_sched_queue *queue;
        mlsl_sched_queue_create(priority_count, comms + idx * priority_count, &queue);
        mlsl_worker_create(e, idx, queue, &(e->workers[idx]));

        if (env_data.worker_offload)
        {
            mlsl_worker_start(e->workers[idx]);
            mlsl_worker_pin(e->workers[idx], env_data.worker_affinity[idx]);
            MLSL_LOG(INFO, "started worker # %zu", idx);
        }
    }

    MLSL_FREE(comms);

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

    mlsl_sched **partial_scheds = sched->partial_scheds;
    size_t partial_sched_count = sched->partial_sched_count;
    MLSL_ASSERTP(partial_scheds && partial_sched_count > 0);

    size_t idx;
    for (idx = 0; idx < partial_sched_count; idx++)
    {
        partial_scheds[idx]->first_progress = 1;
        mlsl_sched_adjust_tag(partial_scheds[idx]);
        mlsl_sched_reset(partial_scheds[idx]);
    }

    if (executor->proc_idx == 0)
        mlsl_sched_dump(sched, "origin_sched");

    /* add scheds into worker queues */
    sched->req->completion_counter = partial_sched_count;
    for (idx = 0; idx < partial_sched_count; idx++)
    {
        if (executor->proc_idx == 0)
            mlsl_sched_dump(partial_scheds[idx], "worker_sched");

        mlsl_sched_queue *queue = executor->workers[idx % executor->worker_count]->sched_queue;
        partial_scheds[idx]->req = sched->req;
        mlsl_sched_queue_add(queue, partial_scheds[idx], mlsl_sched_get_priority(partial_scheds[idx]));
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
