#include <immintrin.h>
#include <sched.h>
#include <stdint.h>
#include "worker.h"

#define MLSL_WORKER_CHECK_CANCEL_ITERS (32768)
#define MLSL_WORKER_YIELD_ITERS        (32768)

static void* mlsl_worker_func(void *args);

mlsl_status_t mlsl_worker_create(mlsl_executor *executor, size_t idx, mlsl_sched_queue *queue, mlsl_worker **worker)
{
    mlsl_worker *w = MLSL_CALLOC(sizeof(mlsl_worker), "worker");
    w->idx = idx;
    w->executor = executor;
    w->sched_queue = queue;
    *worker = w;
    return mlsl_status_success;
}

mlsl_status_t mlsl_worker_free(mlsl_worker *worker)
{
    MLSL_FREE(worker);
    return mlsl_status_success;
}

mlsl_status_t mlsl_worker_start(mlsl_worker *worker)
{
    MLSL_LOG(DEBUG, "worker_idx %zu", worker->idx);
    int err = pthread_create(&(worker->thread), NULL, mlsl_worker_func, (void*)worker);
    if (err)
    {
        MLSL_LOG(ERROR, "error while creating worker thread # %zu, pthread_create returns %d",
                 worker->idx, err);
        return mlsl_status_runtime_error;
    }
    return mlsl_status_success;
}

mlsl_status_t mlsl_worker_stop(mlsl_worker *worker)
{
    MLSL_LOG(DEBUG, "worker_idx %zu", worker->idx);
    int err;
    void *exit_code;
    err = pthread_cancel(worker->thread);
    if (err)
        MLSL_LOG(INFO, "error while canceling progress thread # %zu, pthread_cancel returns %d",
                 worker->idx, err);

    err = pthread_join(worker->thread, &exit_code);
    if (err)
        MLSL_LOG(INFO, "error while joining progress thread # %zu, pthread_join returns %d",
                 worker->idx, err);
    else
        MLSL_LOG(DEBUG, "progress thread # %zu exited with code %ld (%s)",
                 worker->idx, (uintptr_t)exit_code, (exit_code == PTHREAD_CANCELED) ? "PTHREAD_CANCELED" : "?");

    return mlsl_status_success;
}

mlsl_status_t mlsl_worker_pin(mlsl_worker *worker, int proc_id)
{
    MLSL_LOG(DEBUG, "worker_idx %zu, proc_id %d", worker->idx, proc_id);
    int pthread_err;
    cpu_set_t cpuset;

    __CPU_ZERO_S(sizeof(cpu_set_t), &cpuset);
    __CPU_SET_S(proc_id, sizeof(cpu_set_t), &cpuset);

    if ((pthread_err = pthread_setaffinity_np(worker->thread, sizeof(cpu_set_t), &cpuset)) != 0)
    {
        MLSL_LOG(ERROR, "pthread_setaffinity_np failed, err %d", pthread_err);
        return mlsl_status_runtime_error;
    }

    if ((pthread_err = pthread_getaffinity_np(worker->thread, sizeof(cpu_set_t), &cpuset)) != 0)
    {
        MLSL_LOG(ERROR, "pthread_getaffinity_np failed, err %d", pthread_err);
        return mlsl_status_runtime_error;
    }

    if (!__CPU_ISSET_S(proc_id, sizeof(cpu_set_t), &cpuset))
    {
        MLSL_LOG(ERROR, "worker %zu is not pinned on proc_id %d", worker->idx, proc_id);
        return mlsl_status_runtime_error;
    }

    return mlsl_status_success;
}

mlsl_status_t mlsl_worker_peek_and_progress(mlsl_worker *worker, size_t *processed_count)
{
    size_t peek_count;
    mlsl_sched_queue_bin *bin;

    MLSL_ASSERT(processed_count);
    *processed_count = 0;

    mlsl_sched_queue_peek(worker->sched_queue, &bin, &peek_count);
    if (peek_count)
    {
        MLSL_ASSERT(bin);
        mlsl_sched_progress(bin, peek_count, processed_count);
        MLSL_ASSERT(*processed_count <= peek_count);
    }

    return mlsl_status_success;
}

static void* mlsl_worker_func(void *args)
{
    mlsl_worker *worker = (mlsl_worker*)args;
    MLSL_LOG(INFO, "worker_idx %zu", worker->idx);

    size_t iter_count = 0;
    size_t yield_spin_count = 0;
    size_t processed_count = 0;

    do
    {
        mlsl_worker_peek_and_progress(worker, &processed_count);

        iter_count++;
        if ((iter_count % MLSL_WORKER_CHECK_CANCEL_ITERS) == 0)
            pthread_testcancel();

        if (processed_count == 0)
        {
            yield_spin_count++;
            if (yield_spin_count > MLSL_WORKER_YIELD_ITERS)
            {
                yield_spin_count = 0;
                _mm_pause();
            }
        }
        else
            yield_spin_count = 0;
    } while (1);
}
