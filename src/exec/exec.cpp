#include "exec/exec.hpp"
#include "exec/worker.hpp"
#include "exec/service_worker.hpp"

mlsl_executor::mlsl_executor(const mlsl_env_data& env_vars,
                             const mlsl_global_data& global_data)
{
    auto worker_count = static_cast<size_t>(env_vars.worker_count);
    workers.reserve(worker_count);
    auto comm_count = worker_count;

    if (env_vars.priority_mode != mlsl_priority_none)
    {
        comm_count *= MLSL_PRIORITY_BUCKET_COUNT;
    }

    atl_attr_t attr =
    {
        .comm_count = comm_count,
        .enable_rma = env_vars.enable_rma
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
        std::unique_ptr<mlsl_sched_queue> data_queue{new mlsl_sched_queue(comm_vec)};

        if (env_vars.enable_fusion && idx == 0)
        {
            MLSL_LOG(DEBUG, "creating service worker");
            workers.emplace_back(new mlsl_service_worker(this, idx,
                std::move(data_queue), *global_data.fusion_manager));
        }
        else
        {
            workers.emplace_back(new mlsl_worker(this, idx, std::move(data_queue)));
        }

        if (env_vars.worker_offload)
        {
            workers.back()->start();
            workers.back()->pin(env_vars.worker_affinity[idx]);
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
    if (sched->is_internal)
    {
        MLSL_ASSERT_FMT(sched->partial_scheds.empty(), "internal sched should not have partial scheds");
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
