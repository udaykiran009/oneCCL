#pragma once

#include "common/env/env.hpp"
#include "common/request/request.hpp"
#include "atl/atl.h"

#include <vector>
#include <memory>

class mlsl_worker;
class mlsl_service_worker;
class mlsl_sched;

class alignas(CACHELINE_SIZE) mlsl_executor
{
public:
    mlsl_executor() = delete;
    mlsl_executor(const mlsl_executor& other) = delete;
    mlsl_executor& operator= (const mlsl_executor& other) = delete;
    mlsl_executor(mlsl_executor&& other) = delete;
    mlsl_executor& operator= (mlsl_executor&& other) = delete;

    mlsl_executor(size_t workers_count,
                  size_t priority_count,
                  bool service_support,
                  int* workers_affinity = nullptr,
                  mlsl_priority_mode priority_mode = mlsl_priority_none);
    ~mlsl_executor();
    void start_sched(mlsl_sched* sched);
    void wait(mlsl_request* req);
    bool test(mlsl_request* req);

    size_t proc_idx{};
    size_t proc_count{};
    atl_desc_t* atl_desc = nullptr;

    /* TODO: group output ATL parameters */
    int is_rma_enabled = 0;
    size_t max_order_waw_size = 0;

private:
    atl_comm_t** atl_comms = nullptr;

    std::vector<std::unique_ptr<mlsl_worker>> workers;
};


//free functions

inline void mlsl_wait_impl(mlsl_executor* exec, mlsl_request* request)
{
    exec->wait(request);
    MLSL_ASSERT(request->sched, "empty sched");

    MLSL_LOG(INFO, "req %p completed, sched %s", request,
             mlsl_coll_type_to_str(request->sched->coll_param.ctype));

    if (!request->sched->coll_attr.to_cache)
        delete request->sched;
}

inline bool mlsl_test_impl(mlsl_executor* exec, mlsl_request* request)
{
    bool completed = exec->test(request);

    if (completed)
    {
        MLSL_ASSERT(request->sched, "empty sched");
        MLSL_LOG(INFO, "req %p completed, sched %s", request,
                 mlsl_coll_type_to_str(request->sched->coll_param.ctype));

        if (!request->sched->coll_attr.to_cache)
            delete request->sched;
    }

    return completed;
}
