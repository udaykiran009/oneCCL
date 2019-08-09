#include "exec/exec.hpp"
#include "exec/worker.hpp"
#include "exec/service_worker.hpp"

ccl_executor::ccl_executor()
{
    auto worker_count = env_data.worker_count;
    workers.reserve(worker_count);
    auto comm_count = worker_count;

    if (env_data.priority_mode != ccl_priority_none)
    {
        comm_count *= CCL_PRIORITY_BUCKET_COUNT;
    }

    atl_attr_t attr =
    {
        .comm_count = comm_count,
        .enable_rma = env_data.enable_rma
    };

    LOG_INFO("init ATL, requested comm_count ", attr.comm_count);

    atl_status_t atl_status = atl_init(ccl_atl_transport_to_str(env_data.atl_transport),
                                       nullptr, nullptr,
                                       &proc_idx, &proc_count,
                                       &attr, &atl_comms, &atl_desc);
    CCL_THROW_IF_NOT(atl_status == atl_status_success && atl_desc && atl_comms,
                     "ATL init failed, res ", atl_status, ", desc ", atl_desc, ", comm ",atl_comms);

    is_tagged_coll_enabled = attr.is_tagged_coll_enabled;
    tag_bits = attr.tag_bits;
    max_tag = attr.max_tag;
    is_rma_enabled = attr.enable_rma;
    max_order_waw_size = attr.max_order_waw_size;

    LOG_INFO("proc_idx ", proc_idx, ", proc_count ", proc_count,
        ", worker_count ", worker_count);

    if (proc_idx == 0)
    {
        LOG_INFO("\nATL parameters:",
            "\n  comm_count:             ", comm_count,
            "\n  is_tagged_coll_enabled: ", is_tagged_coll_enabled,
            "\n  tag_bits:               ", tag_bits,
            "\n  max_tag:                ", max_tag,
            "\n  is_rma_enabled:         ", is_rma_enabled,
            "\n  max_order_waw_size:     ", max_order_waw_size);
    }

    size_t comm_per_worker = comm_count / worker_count;
    for (size_t idx = 0; idx < worker_count; idx++)
    {
        std::vector<atl_comm_t*> comm_vec(atl_comms + idx * comm_per_worker,
                                          atl_comms + (idx + 1) * comm_per_worker);
        std::unique_ptr<ccl_sched_queue> data_queue{new ccl_sched_queue(comm_vec)};

        if (env_data.enable_fusion && idx == 0)
        {
            LOG_DEBUG("create service worker");
            workers.emplace_back(new ccl_service_worker(this, idx, std::move(data_queue),
                                                        *global_data.fusion_manager));
        }
        else
        {
            workers.emplace_back(new ccl_worker(this, idx, std::move(data_queue)));
        }

        if (env_data.worker_offload)
        {
            workers.back()->start();
            workers.back()->pin(env_data.worker_affinity[idx]);
            LOG_DEBUG("started worker # ", idx);
        }
    }
}

ccl_executor::~ccl_executor()
{
    for (size_t idx = 0; idx < workers.size(); idx++)
    {
        if (env_data.worker_offload)
        {
            workers[idx]->stop();
            LOG_DEBUG("stopped worker # ", idx);
        }
    }

    if (proc_idx == 0)
        LOG_INFO("finalizing ATL");

    auto result = atl_finalize(atl_desc, atl_comms);

    if (proc_idx == 0)
        LOG_INFO("finalized ATL");

    if (result != atl_status_success)
    {
        LOG_ERROR("ATL finalize failed, error ", result);
    }
}

void ccl_executor::start(ccl_sched* sched)
{
    if (sched->internal_type == ccl_sched_internal_unordered_coll)
    {
        CCL_ASSERT(sched->partial_scheds.empty(), "unordered_coll internal sched should not have partial scheds");
        workers[0]->add(sched);
    }
    else
    {
        size_t worker_idx;
        for (size_t idx = 0; idx < sched->partial_scheds.size(); idx++)
        {
            if (env_data.atl_transport == ccl_atl_mpi && sched->coll_param.ctype == ccl_coll_sparse_allreduce)
            {
                /* to workaround issue with multi-threaded MPI_Iprobe support in IMPI */
                worker_idx = 0;
            }
            else
            {
                worker_idx = sched->partial_scheds[idx]->sched_id % workers.size();
            }

            workers[worker_idx]->add(sched->partial_scheds[idx].get());
        }
    }
}

void ccl_executor::wait(ccl_request* req)
{
    req->sched->urgent = true;
    while (!req->is_completed())
    {
        if (env_data.worker_offload)
        {
            ccl_yield(env_data.yield_type);
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

bool ccl_executor::test(ccl_request* req)
{
    bool completed = false;

    if (!req->is_completed())
    {
        req->sched->urgent = true;
        if (env_data.worker_offload)
        {
            ccl_yield(env_data.yield_type);
        }
        else
        {
            for (auto& worker : workers)
            {
                worker->do_work();
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
