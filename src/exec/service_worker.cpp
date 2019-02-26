#include "common/global/global.hpp"
#include "exec/service_worker.hpp"

#define MLSL_WORKER_SERVICE_QUEUE_SKIP_ITERS (128)

mlsl_service_worker::mlsl_service_worker(mlsl_executor* executor, size_t idx,
                                         std::unique_ptr<mlsl_sched_queue> data_queue,
                                         std::unique_ptr<mlsl_sched_queue> service_queue)
    : mlsl_worker(executor, idx, std::move(data_queue)),
      service_queue(std::move(service_queue))
{}

void mlsl_service_worker::add(mlsl_sched* sched)
{
    MLSL_LOG(DEBUG, "add sched %p type %s", sched, mlsl_coll_type_to_str(sched->coll_param.ctype));
    switch (sched->coll_param.ctype)
    {
        case mlsl_coll_service_temporal:
            MLSL_LOG(DEBUG, "Add temp sched %p to service queue", sched->root);
            register_temporal_service_sched(sched->root);
            service_queue->add(sched, sched->get_priority());
            break;
        case mlsl_coll_service_persistent:
            MLSL_LOG(DEBUG, "Add persistent sched %p to service queue", sched->root);
            register_persistent_service_sched(sched->root);
            service_queue->add(sched, sched->get_priority());
            break;
        default:
            mlsl_worker::add(sched);
    }
}

size_t mlsl_service_worker::do_work()
{
    /* TODO: call single method from ooo module */
    if (service_queue_skip_count >= MLSL_WORKER_SERVICE_QUEUE_SKIP_ITERS)
    {
        peek_service();
        service_queue_skip_count = 0;
    }
    else
    {
        ++service_queue_skip_count;
    }

    if (env_data.enable_fusion)
    {
        global_data.fusion_manager->execute();
    }

    return mlsl_worker::do_work();
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
        mlsl_sched_progress(bin, peek_count, processed_count);
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
            service_sched->prepare_partial_scheds(false);
            for (size_t idx = 0; idx < service_sched->partial_scheds.size(); ++idx)
            {
                service_queue->add(service_sched->partial_scheds[idx].get(), service_sched.get()->get_priority());
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
