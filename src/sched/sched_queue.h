#ifndef SCHED_QUEUE_H
#define SCHED_QUEUE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "sched/sched.h"

#define MLSL_SCHED_QUEUE_MAX_BINS (64)

struct mlsl_sched_queue_bin
{
    struct mlsl_sched_queue *queue;
    size_t priority;
    mlsl_sched *elems;
    size_t elem_count;

    atl_comm_t *comm_ctx;

    // TODO:
    void *comp_ctx;
};
typedef struct mlsl_sched_queue_bin mlsl_sched_queue_bin;

struct mlsl_sched_queue
{
    mlsl_fastlock_t lock;
    size_t used_bins;
    size_t max_bins;
    size_t max_priority;
    mlsl_sched_queue_bin bins[MLSL_SCHED_QUEUE_MAX_BINS];
};
typedef struct mlsl_sched_queue mlsl_sched_queue;

mlsl_status_t mlsl_sched_queue_create(size_t max_bins, atl_comm_t **comm_ctxs, mlsl_sched_queue **queue);
mlsl_status_t mlsl_sched_queue_free(mlsl_sched_queue *queue);
mlsl_status_t mlsl_sched_queue_add(mlsl_sched_queue *queue, mlsl_sched *sched, size_t priority);
mlsl_status_t mlsl_sched_queue_remove(mlsl_sched_queue *queue, mlsl_sched_queue_bin *bin, mlsl_sched *sched);
mlsl_status_t mlsl_sched_queue_peek(mlsl_sched_queue *queue, mlsl_sched_queue_bin **bin, size_t *count);
mlsl_status_t mlsl_sched_queue_get_max_priority(mlsl_sched_queue *queue, size_t *priority);

#ifdef __cplusplus
}
#endif

#endif /* SCHED_QUEUE_H */
