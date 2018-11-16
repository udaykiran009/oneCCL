#pragma once

#include "exec/exec.hpp"
#include "sched/sched_queue.hpp"
#include <pthread.h>

struct mlsl_worker
{
    pthread_t thread;
    size_t idx;
    mlsl_executor *executor;
    mlsl_sched_queue *sched_queue;
    size_t empty_progress_counter;
};

mlsl_status_t mlsl_worker_create(mlsl_executor *executor, size_t idx,
                                 mlsl_sched_queue *queue, mlsl_worker **worker);
mlsl_status_t mlsl_worker_free(mlsl_worker *worker);
mlsl_status_t mlsl_worker_start(mlsl_worker *worker);
mlsl_status_t mlsl_worker_stop(mlsl_worker *worker);
mlsl_status_t mlsl_worker_pin(mlsl_worker *worker, int proc_id);
mlsl_status_t mlsl_worker_peek_and_progress(mlsl_worker *worker, size_t *processed_count);
