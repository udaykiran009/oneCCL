#include "common/env/env.hpp"
#include "common/global/global.hpp"
#include "exec/worker.hpp"

#define CCL_WORKER_CHECK_CANCEL_ITERS (32768)

static void* ccl_worker_func(void* args);

ccl_worker::ccl_worker(ccl_executor* executor,
                       size_t idx,
                       std::unique_ptr<ccl_sched_queue> queue)
    : idx(idx), executor(executor), data_queue(std::move(queue))
{}

ccl_status_t ccl_worker::start()
{
    LOG_DEBUG("worker_idx ", idx);
    int err = pthread_create(&thread, nullptr, ccl_worker_func, static_cast<void*>(this));
    if (err)
    {
        LOG_ERROR("error while creating worker thread #", idx, " pthread_create returns ", err);
        return ccl_status_runtime_error;
    }
    return ccl_status_success;
}

ccl_status_t ccl_worker::stop()
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
    return ccl_status_success;
}

ccl_status_t ccl_worker::pin(int proc_id)
{
    LOG_DEBUG("worker_idx ", idx, ", proc_id ", proc_id);
    int pthread_err;
    cpu_set_t cpuset;

    __CPU_ZERO_S(sizeof(cpu_set_t), &cpuset);
    __CPU_SET_S(proc_id, sizeof(cpu_set_t), &cpuset);

    if ((pthread_err = pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset)) != 0)
    {
        LOG_ERROR("pthread_setaffinity_np failed, err ", pthread_err);
        return ccl_status_runtime_error;
    }

    if ((pthread_err = pthread_getaffinity_np(thread, sizeof(cpu_set_t), &cpuset)) != 0)
    {
        LOG_ERROR("pthread_getaffinity_np failed, err ", pthread_err);
        return ccl_status_runtime_error;
    }

    if (!__CPU_ISSET_S(proc_id, sizeof(cpu_set_t), &cpuset))
    {
        LOG_ERROR("worker ", idx, " is not pinned on proc_id ", proc_id);
        return ccl_status_runtime_error;
    }

    return ccl_status_success;
}

void ccl_worker::add(ccl_sched* sched)
{
    LOG_DEBUG("add sched ", sched, ", type ", ccl_coll_type_to_str(sched->coll_param.ctype));
    data_queue->add(sched);
}

size_t ccl_worker::do_work()
{
    size_t processed_count = 0;
    ccl_sched_bin* bin = data_queue->peek();
    if (bin)
    {
        ccl_bin_progress(bin, processed_count);
    }

    return processed_count;
}

static void* ccl_worker_func(void* args)
{
    auto worker = static_cast<ccl_worker*>(args);
    LOG_DEBUG("worker_idx ", worker->get_idx());

    size_t iter_count = 0;
    size_t processed_count = 0;
    size_t spin_count = env_data.spin_count;
    size_t max_spin_count = env_data.spin_count;

    global_data.is_worker_thread = true;

    do
    {
        try
        {
            processed_count = worker->do_work();
        }
        catch (ccl::ccl_error& ccl_e)
        {
            CCL_FATAL("worker ", worker->get_idx(), " caught internal exception: ", ccl_e.what());
        }
        catch (std::exception& e)
        {
            CCL_FATAL("worker ", worker->get_idx(), " caught exception: ", e.what());
        }
        catch (...)
        {
            CCL_FATAL("worker ", worker->get_idx(), " caught general exception");
        }

        iter_count++;
        if ((iter_count % CCL_WORKER_CHECK_CANCEL_ITERS) == 0)
        {
            pthread_testcancel();
        }

        if (processed_count == 0)
        {
            spin_count--;
            if (!spin_count)
            {
                ccl_yield(env_data.yield_type);
                spin_count = 1;
            }
        }
        else
        {
            spin_count = max_spin_count;
        }
    } while (true);

    return nullptr;
}
