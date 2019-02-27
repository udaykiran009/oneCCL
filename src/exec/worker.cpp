#include "common/global/global.hpp"
#include "exec/worker.hpp"
#include "sched/sched.hpp"

#include <stdint.h>

#define MLSL_WORKER_CHECK_CANCEL_ITERS (32768)
#define MLSL_WORKER_YIELD_ITERS        (32768)

static void* mlsl_worker_func(void* args);

mlsl_worker::mlsl_worker(mlsl_executor* executor,
                         size_t idx,
                         std::unique_ptr<mlsl_sched_queue> queue)
    : idx(idx), executor(executor), data_queue(std::move(queue))
{}

mlsl_status_t mlsl_worker::start()
{
    MLSL_LOG(DEBUG, "worker_idx %zu", idx);
    int err = pthread_create(&thread, nullptr, mlsl_worker_func, static_cast<void*>(this));
    if (err)
    {
        MLSL_LOG(ERROR, "error while creating worker thread # %zu, pthread_create returns %d",
                 idx, err);
        return mlsl_status_runtime_error;
    }
    return mlsl_status_success;
}

mlsl_status_t mlsl_worker::stop()
{
    MLSL_LOG(DEBUG, "worker_idx %zu", idx);
    void* exit_code;
    int err = pthread_cancel(thread);
    if (err)
        MLSL_LOG(INFO, "error while canceling progress thread # %zu, pthread_cancel returns %d",
                 idx, err);

    err = pthread_join(thread, &exit_code);
    if (err)
        MLSL_LOG(INFO, "error while joining progress thread # %zu, pthread_join returns %d",
                 idx, err);
    else
        MLSL_LOG(DEBUG, "progress thread # %zu exited with code %ld (%s)",
                 idx, (uintptr_t) exit_code, (exit_code == PTHREAD_CANCELED) ? "PTHREAD_CANCELED" : "?");
    return mlsl_status_success;
}

mlsl_status_t mlsl_worker::pin(int proc_id)
{
    MLSL_LOG(DEBUG, "worker_idx %zu, proc_id %d", idx, proc_id);
    int pthread_err;
    cpu_set_t cpuset;

    __CPU_ZERO_S(sizeof(cpu_set_t), &cpuset);
    __CPU_SET_S(proc_id, sizeof(cpu_set_t), &cpuset);

    if ((pthread_err = pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset)) != 0)
    {
        MLSL_LOG(ERROR, "pthread_setaffinity_np failed, err %d", pthread_err);
        return mlsl_status_runtime_error;
    }

    if ((pthread_err = pthread_getaffinity_np(thread, sizeof(cpu_set_t), &cpuset)) != 0)
    {
        MLSL_LOG(ERROR, "pthread_getaffinity_np failed, err %d", pthread_err);
        return mlsl_status_runtime_error;
    }

    if (!__CPU_ISSET_S(proc_id, sizeof(cpu_set_t), &cpuset))
    {
        MLSL_LOG(ERROR, "worker %zu is not pinned on proc_id %d", idx, proc_id);
        return mlsl_status_runtime_error;
    }

    return mlsl_status_success;
}

void mlsl_worker::add(mlsl_sched* sched)
{
    MLSL_LOG(DEBUG, "add sched %p type %s", sched, mlsl_coll_type_to_str(sched->coll_param.ctype));
    data_queue->add(sched, sched->get_priority());
}

size_t mlsl_worker::do_work()
{
    size_t peek_count;
    size_t processed_count = 0;
    mlsl_sched_queue_bin* bin = data_queue->peek(peek_count);

    if (peek_count)
    {
        MLSL_ASSERT(bin);
        mlsl_sched_progress(bin, peek_count, processed_count);
        MLSL_ASSERT(processed_count <= peek_count);
    }

    return processed_count;
}

static void* mlsl_worker_func(void* args)
{
    auto worker = static_cast<mlsl_worker*>(args);
    MLSL_LOG(INFO, "worker_idx %zu", worker->get_idx());

    size_t iter_count = 0;
    size_t yield_spin_count = 0;
    size_t processed_count = 0;

    global_data.is_worker_thread = true;

    do
    {
        processed_count = worker->do_work();

        iter_count++;
        if ((iter_count % MLSL_WORKER_CHECK_CANCEL_ITERS) == 0)
        {
            pthread_testcancel();
        }

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
        {
            yield_spin_count = 0;
        }
    } while (true);
}
