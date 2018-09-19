#include "sched_queue.h"

mlsl_status_t mlsl_sched_queue_create(size_t max_bins, atl_comm_t **comm_ctxs, mlsl_sched_queue **queue)
{
    MLSL_ASSERTP(max_bins <= MLSL_SCHED_QUEUE_MAX_BINS);
    mlsl_sched_queue *q = MLSL_CALLOC(sizeof(mlsl_sched_queue), "schedule queue");
    mlsl_fastlock_init(&q->lock);
    for (size_t idx = 0; idx < max_bins; idx++)
    {
        q->bins[idx].queue = q;
        q->bins[idx].comm_ctx = comm_ctxs[idx];
        MLSL_LOG(DEBUG, "comm_ctxs[%zu]: %p", idx, comm_ctxs[idx]);
    }
    q->max_bins = max_bins;

    *queue = q;

    return mlsl_status_success;
}

mlsl_status_t mlsl_sched_queue_free(mlsl_sched_queue *queue)
{
    mlsl_fastlock_destroy(&queue->lock);
    MLSL_ASSERTP(queue->used_bins == 0);
    MLSL_ASSERTP(queue->max_priority == 0);
    MLSL_FREE(queue);

    return mlsl_status_success;
}

mlsl_status_t mlsl_sched_queue_add(mlsl_sched_queue *queue, mlsl_sched *sched, size_t priority)
{
    mlsl_fastlock_acquire(&queue->lock);
    mlsl_sched_queue_bin *bin = &(queue->bins[priority % queue->max_bins]);
    if (!bin->elems)
    {
        MLSL_ASSERT(bin->priority == 0);
        MLSL_ASSERT(bin->elem == 0);
        bin->priority = priority;
        queue->used_bins++;
    }
    MLSL_DLIST_APPEND(bin->elems, sched);
    sched->bin = bin;
    bin->elem_count++;
    queue->max_priority = MAX(queue->max_priority, priority);
    mlsl_fastlock_release(&queue->lock);

    return mlsl_status_success;
}

mlsl_status_t mlsl_sched_queue_remove(mlsl_sched_queue *queue, mlsl_sched_queue_bin *bin, mlsl_sched *sched)
{
    MLSL_LOG(DEBUG, "queue %p, bin %p, elems %p, sched %p, count %zu",
             queue, bin, bin->elems, sched, bin->elem_count);

    mlsl_fastlock_acquire(&queue->lock);
    MLSL_DLIST_DELETE(bin->elems, sched);
    bin->elem_count--;
    MLSL_ASSERT(bin->elem_count >= 0);
    if (bin->elem_count == 0) MLSL_ASSERT(!bin->elems);
    if (!bin->elems)
    {
        queue->used_bins--;
        if (queue->used_bins == 0) queue->max_priority = 0;
        else
        {
            size_t bin_idx = queue->max_priority % queue->max_bins;
            while (!queue->bins[bin_idx].elems)
            {
                bin_idx = (bin_idx - 1 + queue->max_bins) % queue->max_bins;
            }
            queue->max_priority = queue->bins[bin_idx].priority;
        }
        bin->priority = 0;
    }
    mlsl_fastlock_release(&queue->lock);

    return mlsl_status_success;
}

mlsl_status_t mlsl_sched_queue_peek(mlsl_sched_queue *queue, mlsl_sched_queue_bin **bin, size_t *count)
{
    mlsl_fastlock_acquire(&queue->lock);
    if (queue->used_bins > 0)
    {
        *bin = &(queue->bins[queue->max_priority % queue->max_bins]);
        *count = (*bin)->elem_count;
        MLSL_ASSERT(*count > 0);
    }
    else
    {
        *bin = NULL;
        *count = 0;
    }
    mlsl_fastlock_release(&queue->lock);

    return mlsl_status_success;
}

mlsl_status_t mlsl_sched_queue_get_max_priority(mlsl_sched_queue *queue, size_t *priority)
{
    *priority = queue->max_priority;
    return mlsl_status_success;
}
