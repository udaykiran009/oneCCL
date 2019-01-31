#include "exec/exec.hpp"
#include "exec/worker.hpp"
#include "common/global/global.hpp"
#include "sched/sched_queue.hpp"
#include "common/utils/utils.hpp"

const size_t MLSL_ATL_MAX_COMMS = 8;

mlsl_executor::mlsl_executor(size_t workers_count, size_t priority_count, bool service_support, int* workers_affinity, mlsl_priority_mode priority_mode)
{
    workers.reserve(workers_count);
    size_t total_comm_count = workers_count * priority_count;
    size_t comm_count = std::min(total_comm_count, MLSL_ATL_MAX_COMMS);

    if (priority_mode == mlsl_priority_none)
    {
        comm_count = workers_count;
    }

    MLSL_ASSERTP(comm_count >= workers_count);
    atl_attr_t attr =
    {
        .comm_count = comm_count,
        .enable_rma = env_data.enable_rma
    };
    MLSL_LOG(INFO, "atl comm count %zu", attr.comm_count);

    if (service_support)
    {
        MLSL_LOG(DEBUG, "additional atl comm will be requested");
        ++attr.comm_count;
    }

    atl_status_t atl_status = atl_init(nullptr, nullptr, &proc_idx, &proc_count, &attr, &atl_comms, &atl_desc);
    MLSL_ASSERTP(atl_status == atl_status_success);
    MLSL_ASSERTP(atl_desc);
    MLSL_ASSERTP(atl_comms);

    /* atl will return back whether rma is supported */
    is_rma_enabled = attr.enable_rma;
    max_order_waw_size = attr.max_order_waw_size;

    MLSL_LOG(INFO, "proc_idx %zu, proc_count %zu, atl_desc %p",
             proc_idx, proc_count, atl_desc);

    MLSL_LOG(INFO, "atl parameters: is_rma_enabled %d, max_order_waw_size %zu",
             is_rma_enabled, max_order_waw_size);

    std::vector<atl_comm_t*> comms(total_comm_count, nullptr);
    size_t comms_per_worker = comm_count / workers_count;
    size_t priorities_per_comm = priority_count / comms_per_worker;
    size_t pr_idx;
    for (size_t idx = 0; idx < workers_count; idx++)
    {
        for (pr_idx = 0; pr_idx < priority_count; pr_idx++)
        {
            comms[idx * priority_count + pr_idx] = atl_comms[idx * comms_per_worker + pr_idx / priorities_per_comm];
            if (proc_idx == 0)
                MLSL_LOG(DEBUG, "map atl comms: w_idx %zu, pr_idx %zu, comm_idx %zu, comms_per_worker %zu",
                         idx, pr_idx, idx * comms_per_worker + pr_idx / priorities_per_comm, comms_per_worker);
        }
    }

    for (size_t idx = 0; idx < workers_count; idx++)
    {
        std::unique_ptr<mlsl_sched_queue> data_queue{
            new mlsl_sched_queue(priority_count, comms.data() + idx * priority_count)
        };
        if (service_support && idx == 0)
        {
            std::unique_ptr<mlsl_sched_queue> service_queue{new mlsl_sched_queue{1, &(atl_comms[attr.comm_count - 1])}};
            MLSL_LOG(DEBUG, "Creating service worker #0");
            workers.emplace_back(new mlsl_service_worker{this, idx, std::move(data_queue), std::move(service_queue)});
        }
        else
        {
            workers.emplace_back(new mlsl_worker(this, idx, std::move(data_queue)));
        }

        if (env_data.worker_offload)
        {
            workers.back()->start();
            workers.back()->pin_to_proc(workers_affinity[idx]);
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
            //todo: move to worker's destructor
            workers[idx]->stop();
            MLSL_LOG(DEBUG, "stopped worker # %zu", idx);
        }
    }

    atl_finalize(atl_desc, atl_comms);
}

void mlsl_executor::start_sched(mlsl_sched* sched)
{
    MLSL_ASSERTP_FMT((sched->coll_param.ctype != mlsl_coll_service_temporal &&
                      sched->coll_param.ctype != mlsl_coll_service_persistent) ||
                     sched->partial_sched_count == 1,
                     "Service schedule must have exactly 1 entry");

    mlsl_sched_prepare(sched, proc_idx == 0);

    /* add scheds into worker queues */
    for (size_t idx = 0; idx < sched->partial_sched_count; idx++)
    {
        workers[idx % workers.size()]->add_to_queue(sched->partial_scheds[idx]);
    }
}

void mlsl_executor::wait(mlsl_request* req)
{
    MLSL_LOG(DEBUG, "req %p, req->cc %d", req, req->completion_counter);

    while (__atomic_load_n(&req->completion_counter, __ATOMIC_ACQUIRE))
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
                workers[idx]->peek_and_progress();
            }
        }
    }
}

bool mlsl_executor::test(mlsl_request* req)
{
    bool completed = false;

    MLSL_LOG(DEBUG, "req %p, req->cc %d", req, req->completion_counter);

    size_t completion_counter = __atomic_load_n(&req->completion_counter, __ATOMIC_ACQUIRE);
    if (completion_counter)
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
                workers[idx]->peek_and_progress();
            }
        }
        completed = __atomic_load_n(&req->completion_counter, __ATOMIC_ACQUIRE) == 0;
    }
    else
    {
        completed = true;
    }

    return completed;
}
