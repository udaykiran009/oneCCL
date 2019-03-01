#include "exec/exec.hpp"
#include "exec/worker.hpp"
#include "exec/service_worker.hpp"
#include "common/global/global.hpp"
#include "sched/sched_queue.hpp"
#include "common/utils/utils.hpp"

mlsl_executor::mlsl_executor(size_t worker_count, int* worker_affinity, bool create_service_worker)
{
    workers.reserve(worker_count);

    size_t comm_count = worker_count;
    if (env_data.priority_mode != mlsl_priority_none)
    {
        comm_count *= MLSL_PRIORITY_BUCKET_COUNT;
    }

    atl_attr_t attr =
    {
        .comm_count = comm_count,
        .enable_rma = env_data.enable_rma
    };

    MLSL_LOG(INFO, "atl comm count %zu", attr.comm_count);

    atl_status_t atl_status = atl_init(nullptr, nullptr,
                                       &proc_idx, &proc_count,
                                       &attr, &atl_comms, &atl_desc);
    MLSL_THROW_IF_NOT(atl_status == atl_status_success && atl_desc && atl_comms,
                      "ATL init failed, res %d, desc %p, comm %p",
                      atl_status, atl_desc, atl_comms);

    /* atl will return back whether rma is supported */
    is_rma_enabled = attr.enable_rma;
    max_order_waw_size = attr.max_order_waw_size;

    MLSL_LOG(INFO, "proc_idx %zu, proc_count %zu, atl_desc %p",
             proc_idx, proc_count, atl_desc);

    MLSL_LOG(INFO, "atl parameters: is_rma_enabled %d, max_order_waw_size %zu",
             is_rma_enabled, max_order_waw_size);

    size_t comm_per_worker = comm_count / worker_count;
    for (size_t idx = 0; idx < worker_count; idx++)
    {
        std::vector<atl_comm_t*> comm_vec(atl_comms + idx * comm_per_worker,
                                          atl_comms + (idx + 1) * comm_per_worker);
        std::unique_ptr<mlsl_sched_queue> data_queue
        {
            new mlsl_sched_queue(comm_vec)
        };

        if (create_service_worker && idx == 0)
        {
            std::unique_ptr<mlsl_sched_queue> service_queue
            {
                new mlsl_sched_queue(comm_vec)
            };
            MLSL_LOG(DEBUG, "creating service worker");
            workers.emplace_back(new mlsl_service_worker(this, idx,
                std::move(data_queue), std::move(service_queue)));
        }
        else
        {
            workers.emplace_back(new mlsl_worker(this, idx, std::move(data_queue)));
        }

        if (env_data.worker_offload)
        {
            workers.back()->start();
            workers.back()->pin(worker_affinity[idx]);
            MLSL_LOG(DEBUG, "started worker #%zu", idx);
        }
    }
}

mlsl_executor::~mlsl_executor()
{
    for (size_t idx = 0; idx < workers.size(); idx++)
    {
        if (env_data.worker_offload)
        {
            workers[idx]->stop();
            MLSL_LOG(DEBUG, "stopped worker # %zu", idx);
        }
    }

    MLSL_LOG(DEBUG, "finalizing ATL..");
    atl_finalize(atl_desc, atl_comms);
}

void mlsl_executor::start(mlsl_sched* sched)
{
    MLSL_ASSERT_FMT((sched->coll_param.ctype != mlsl_coll_service_temporal &&
                     sched->coll_param.ctype != mlsl_coll_service_persistent) ||
                     sched->partial_scheds.size() == 1,
                     "Service schedule must have exactly 1 entry");

    /* add scheds into worker queues */
    if (sched->partial_scheds.empty())
    {
        /* TODO: use regular non-partial schedue for ooo module */
        MLSL_FATAL("empty sched is not supported"); // for now
        workers[0]->add(sched);
    }
    else
    {
        for (size_t idx = 0; idx < sched->partial_scheds.size(); idx++)
        {
            workers[idx % workers.size()]->add(sched->partial_scheds[idx].get());
        }
    }
}

void mlsl_executor::wait(mlsl_request* req)
{
    req->sched->urgent = true;
    while (!req->is_completed())
    {
        if (env_data.worker_offload)
        {
            _mm_pause();
        }
        else
        {
            size_t idx;
            for (idx = 0; idx < workers.size(); idx++)
            {
                workers[idx]->do_work();
            }
        }
    }
    req->sched->urgent = false;
}

bool mlsl_executor::test(mlsl_request* req)
{
    bool completed = false;

    if (!req->is_completed())
    {
        req->sched->urgent = true;
        if (env_data.worker_offload)
        {
            _mm_pause();
        }
        else
        {
            size_t idx;
            for (idx = 0; idx < workers.size(); idx++)
            {
                workers[idx]->do_work();
            }
        }
    }
    else
    {
        completed = true;
    }

    if (completed)
        req->sched->urgent = false;

    return completed;
}
