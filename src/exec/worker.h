#ifndef WORKER_H
#define WORKER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "exec/exec.h"
#include "sched/sched_queue.h"
#include <pthread.h>

struct mlsl_worker
{
    pthread_t thread;
    size_t idx;
    struct mlsl_executor *executor;
    mlsl_sched_queue *sched_queue;
    size_t empty_progress_counter;
};

typedef struct mlsl_worker mlsl_worker;

mlsl_status_t mlsl_worker_create(struct mlsl_executor *executor, size_t idx,
                                 mlsl_sched_queue *queue, mlsl_worker **worker);
mlsl_status_t mlsl_worker_free(mlsl_worker *worker);
mlsl_status_t mlsl_worker_start(mlsl_worker *worker);
mlsl_status_t mlsl_worker_stop(mlsl_worker *worker);
mlsl_status_t mlsl_worker_pin(mlsl_worker *worker, int proc_id);
mlsl_status_t mlsl_worker_peek_and_progress(mlsl_worker *worker, size_t *processed_count);

#ifdef __cplusplus
}
#endif

#endif /* WORKER_H */
