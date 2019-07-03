#pragma once

#include "common/env/env.hpp"
#include "common/request/request.hpp"
#include "common/global/global.hpp"
#include "atl/atl.h"

#include <vector>
#include <memory>

class iccl_worker;
class iccl_service_worker;
class iccl_sched;

class alignas(CACHELINE_SIZE) iccl_executor
{
public:
    iccl_executor() = delete;
    iccl_executor(const iccl_executor& other) = delete;
    iccl_executor& operator=(const iccl_executor& other) = delete;
    iccl_executor(iccl_executor&& other) = delete;
    iccl_executor& operator=(iccl_executor&& other) = delete;

    iccl_executor(const iccl_env_data& env_vars,
                  const iccl_global_data& global_data);
    ~iccl_executor();

    void start(iccl_sched* sched);
    void wait(iccl_request* req);
    bool test(iccl_request* req);

    size_t proc_idx{};
    size_t proc_count{};
    atl_desc_t* atl_desc = nullptr;

    /* TODO: group output ATL parameters */
    bool is_tagged_coll_enabled = false;
    size_t tag_bits = 64;
    uint64_t max_tag = 0xFFFFFFFFFFFFFFFF;
    bool is_rma_enabled = false;
    size_t max_order_waw_size = 0;

private:
    atl_comm_t** atl_comms = nullptr;
    std::vector<std::unique_ptr<iccl_worker>> workers;
};


//free functions

inline void iccl_wait_impl(iccl_executor* exec,
                           iccl_request* request)
{
    exec->wait(request);
    ICCL_ASSERT(request->sched);

    LOG_DEBUG("req ", request, " completed, sched ", iccl_coll_type_to_str(request->sched->coll_param.ctype));

    if (!request->sched->coll_attr.to_cache)
    {
        delete request->sched;
    }
}

inline bool iccl_test_impl(iccl_executor* exec,
                           iccl_request* request)
{
    bool completed = exec->test(request);

    if (completed)
    {
        ICCL_ASSERT(request->sched);
        LOG_DEBUG("req ", request, " completed, sched ", iccl_coll_type_to_str(request->sched->coll_param.ctype));

        if (!request->sched->coll_attr.to_cache)
        {
            delete request->sched;
        }
    }

    return completed;
}
