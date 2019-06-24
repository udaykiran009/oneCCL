#include "common/global/global.hpp"
#include "exec/worker.hpp"

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
    LOG_DEBUG("worker_idx ", idx);
    int err = pthread_create(&thread, nullptr, mlsl_worker_func, static_cast<void*>(this));
    if (err)
    {
        LOG_ERROR("error while creating worker thread #", idx, " pthread_create returns ", err);
        return mlsl_status_runtime_error;
    }
    return mlsl_status_success;
}

mlsl_status_t mlsl_worker::stop()
{
    LOG_DEBUG("worker # ", idx);
    void* exit_code;
    int err = pthread_cancel(thread);
    if (err)
        LOG_INFO("error while canceling progress thread # ", idx, ", pthread_cancel returns ", err);

    err = pthread_join(thread, &exit_code);
    if (err)
    {
        LOG_INFO("error while joining progress thread # ", idx, " , pthread_join returns ", err);
    }
    else
    {
        LOG_DEBUG("progress thread # ", idx, ", exited with code ",
                  idx, " (", (uintptr_t) exit_code, (exit_code == PTHREAD_CANCELED) ? "PTHREAD_CANCELED" : "?", ")");
    }
    return mlsl_status_success;
}

mlsl_status_t mlsl_worker::pin(int proc_id)
{
    LOG_DEBUG("worker_idx ", idx, ", proc_id ", proc_id);
    int pthread_err;
    cpu_set_t cpuset;

    __CPU_ZERO_S(sizeof(cpu_set_t), &cpuset);
    __CPU_SET_S(proc_id, sizeof(cpu_set_t), &cpuset);

    if ((pthread_err = pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset)) != 0)
    {
        LOG_ERROR("pthread_setaffinity_np failed, err ", pthread_err);
        return mlsl_status_runtime_error;
    }

    if ((pthread_err = pthread_getaffinity_np(thread, sizeof(cpu_set_t), &cpuset)) != 0)
    {
        LOG_ERROR("pthread_getaffinity_np failed, err ", pthread_err);
        return mlsl_status_runtime_error;
    }

    if (!__CPU_ISSET_S(proc_id, sizeof(cpu_set_t), &cpuset))
    {
        LOG_ERROR("worker ", idx, " is not pinned on proc_id ", proc_id);
        return mlsl_status_runtime_error;
    }

    return mlsl_status_success;
}

void mlsl_worker::add(mlsl_sched* sched)
{
    LOG_DEBUG("add sched ", sched, ", type ", mlsl_coll_type_to_str(sched->coll_param.ctype));
    data_queue->add(sched);
}

size_t mlsl_worker::do_work()
{
    size_t peek_count;
    size_t processed_count = 0;
    mlsl_sched_bin* bin = data_queue->peek(peek_count);

    if (peek_count)
    {
        MLSL_ASSERT(bin);
        mlsl_bin_progress(bin, peek_count, processed_count);
        MLSL_ASSERT(processed_count <= peek_count, "incorrect values ", processed_count, " ", peek_count);
    }

    return processed_count;
}

static void* mlsl_worker_func(void* args)
{
    auto worker = static_cast<mlsl_worker*>(args);
    LOG_DEBUG("worker_idx ", worker->get_idx());

    size_t iter_count = 0;
    size_t yield_spin_count = 0;
    size_t processed_count = 0;

    global_data.is_worker_thread = true;

    do
    {
        try
        {
            processed_count = worker->do_work();
        }
        catch (mlsl::mlsl_error& mlsl_e)
        {
            MLSL_FATAL("worker ", worker->get_idx(), " caught internal exception: ", mlsl_e.what());
        }
        catch (std::exception& e)
        {
            MLSL_FATAL("worker ", worker->get_idx(), " caught exception: ", e.what());
        }
        catch (...)
        {
            MLSL_FATAL("worker ", worker->get_idx(), " caught general exception");
        }

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

    return nullptr;
}
