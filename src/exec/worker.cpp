#include "exec/worker.hpp"
#include "sched/sched.hpp"

#include <stdint.h>

#define MLSL_WORKER_CHECK_CANCEL_ITERS (32768)
#define MLSL_WORKER_YIELD_ITERS        (32768)
//todo: find better value, maybe MLSL_WORKER_YIELD_ITERS
#define MLSL_WORKER_SERVICE_QUEUE_SKIP_ITERS (128)

static void* mlsl_worker_func(void* args);


mlsl_worker::mlsl_worker(mlsl_executor* executor, size_t idx, std::unique_ptr<mlsl_sched_queue> queue)
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

void mlsl_worker::stop()
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
}

mlsl_status_t mlsl_worker::pin_to_proc(int proc_id)
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

size_t mlsl_worker::peek_and_progress()
{
    size_t peek_count;
    mlsl_sched_queue_bin* bin;
    size_t processed_count = 0;

    bin = data_queue->peek(peek_count);
    if (peek_count)
    {
        MLSL_ASSERT(bin);
        mlsl_sched_progress(bin, peek_count, &processed_count);
        MLSL_ASSERT(processed_count <= peek_count);
    }

    return processed_count;
}

void mlsl_worker::add_to_queue(mlsl_sched* sched)
{
    MLSL_LOG(DEBUG, "add sched %p type %s", sched, mlsl_coll_type_to_str(sched->coll_param.ctype));
    data_queue->add(sched, mlsl_sched_get_priority(sched));
}

/////////////////////////////////////

mlsl_service_worker::mlsl_service_worker(mlsl_executor* executor, size_t idx,
                                         std::unique_ptr<mlsl_sched_queue> data_queue,
                                         std::unique_ptr<mlsl_sched_queue> service_queue)
    : mlsl_worker(executor, idx, std::move(data_queue)),
      service_queue(std::move(service_queue))
{}

void mlsl_service_worker::add_to_queue(mlsl_sched* sched)
{
    MLSL_LOG(DEBUG, "add sched %p type %s", sched, mlsl_coll_type_to_str(sched->coll_param.ctype));
    switch (sched->coll_param.ctype)
    {
        case mlsl_coll_service_temporal:
            MLSL_LOG(DEBUG, "Add temp sched %p to service queue", sched->root);
            register_temporal_service_sched(sched->root);
            service_queue->add(sched, mlsl_sched_get_priority(sched));
            break;
        case mlsl_coll_service_persistent:
            MLSL_LOG(DEBUG, "Add persistent sched %p to service queue", sched->root);
            register_persistent_service_sched(sched->root);
            service_queue->add(sched, mlsl_sched_get_priority(sched));
            break;
        default:
            mlsl_worker::add_to_queue(sched);
    }
}

size_t mlsl_service_worker::peek_and_progress()
{
    if (service_queue_skip_count >= MLSL_WORKER_SERVICE_QUEUE_SKIP_ITERS)
    {
        peek_service();
        service_queue_skip_count = 0;
    }
    else
    {
        ++service_queue_skip_count;
    }

    return mlsl_worker::peek_and_progress();
}

void mlsl_service_worker::peek_service()
{
    size_t peek_count = 0;
    mlsl_sched_queue_bin* bin;
    size_t processed_count = 0;

    bin = service_queue->peek(peek_count);

    if (peek_count > 0)
    {
        MLSL_ASSERT(bin);
        mlsl_sched_progress(bin, peek_count, &processed_count);
        MLSL_ASSERT(processed_count <= peek_count);

        //todo: visitor might be useful there
        check_persistent();
        check_temporal();
    }
}

void mlsl_service_worker::check_persistent()
{
    for (auto& service_sched : persistent_service_scheds)
    {
        size_t completion_counter = __atomic_load_n(&service_sched->req->completion_counter, __ATOMIC_ACQUIRE);
        if (completion_counter == 0)
        {
            MLSL_LOG(DEBUG, "restart persistent service sched %p", service_sched.get());
            mlsl_sched_prepare_partial_scheds(service_sched.get(), false);
            for (size_t idx = 0; idx < service_sched->partial_scheds.size(); ++idx)
            {
                service_queue->add(service_sched->partial_scheds[idx].get(), mlsl_sched_get_priority(service_sched.get()));
            }
        }
    }
}

void mlsl_service_worker::check_temporal()
{
    for (auto it = temporal_service_scheds.begin(); it != temporal_service_scheds.end();)
    {
        size_t completion_counter = __atomic_load_n(&it->get()->req->completion_counter, __ATOMIC_ACQUIRE);
        if (completion_counter == 0)
        {
            MLSL_LOG(DEBUG, "remove completed temporal service sched %p", it->get());
            it = temporal_service_scheds.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void mlsl_service_worker::erase_service_scheds(std::list<std::shared_ptr<mlsl_sched>>& scheds)
{
    //todo: check that worker's thread is stopped at this point
    for (auto it = scheds.begin(); it != scheds.end();)
    {
        for (size_t idx = 0; idx < it->get()->partial_scheds.size(); ++idx)
        {
            mlsl_request_complete(it->get()->partial_scheds[idx]->req);
            it->get()->partial_scheds[idx]->req = nullptr;
            if (it->get()->partial_scheds[idx]->bin)
            {
                service_queue->erase(it->get()->partial_scheds[idx]->bin, it->get()->partial_scheds[idx].get());
            }
        }
        it = scheds.erase(it);
    }
}

mlsl_service_worker::~mlsl_service_worker()
{
    erase_service_scheds(persistent_service_scheds);
    erase_service_scheds(temporal_service_scheds);
}

/////////////////////////////////////

static void* mlsl_worker_func(void* args)
{
    auto worker = static_cast<mlsl_worker*>(args);
    MLSL_LOG(INFO, "worker_idx %zu", worker->idx);

    size_t iter_count = 0;
    size_t yield_spin_count = 0;
    size_t processed_count = 0;

    do
    {
        processed_count = worker->peek_and_progress();

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
