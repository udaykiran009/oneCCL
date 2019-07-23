#pragma once

#include "common/env/env.hpp"
#include "common/request/request.hpp"
#include "common/global/global.hpp"
#include "atl/atl.h"

#include <vector>
#include <memory>

class ccl_worker;
class ccl_service_worker;
class ccl_sched;

class alignas(CACHELINE_SIZE) ccl_executor
{
public:
    ccl_executor() = delete;
    ccl_executor(const ccl_executor& other) = delete;
    ccl_executor& operator=(const ccl_executor& other) = delete;
    ccl_executor(ccl_executor&& other) = delete;
    ccl_executor& operator=(ccl_executor&& other) = delete;

    ccl_executor(const ccl_env_data& env_vars,
                  const ccl_global_data& global_data);
    ~ccl_executor();

    void start(ccl_sched* sched);
    void wait(ccl_request* req);
    bool test(ccl_request* req);

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
    std::vector<std::unique_ptr<ccl_worker>> workers;
};


//free functions

inline void ccl_wait_impl(ccl_executor* exec,
                          ccl_request* request)
{
    exec->wait(request);
    CCL_ASSERT(request->sched);

    LOG_DEBUG("req ", request, " completed, sched ", ccl_coll_type_to_str(request->sched->coll_param.ctype));

    if (!request->sched->coll_attr.to_cache)
    {
        delete request->sched;
    }
}

inline bool ccl_test_impl(ccl_executor* exec,
                           ccl_request* request)
{
    bool completed = exec->test(request);

    if (completed)
    {
        CCL_ASSERT(request->sched);
        LOG_DEBUG("req ", request, " completed, sched ", ccl_coll_type_to_str(request->sched->coll_param.ctype));

        if (!request->sched->coll_attr.to_cache)
        {
            delete request->sched;
        }
    }

    return completed;
}
