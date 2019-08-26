#pragma once

#include "coll/coll.hpp"
#include "common/env/env.hpp"
#include "common/global/global.hpp"
#include "common/request/request.hpp"
#include "atl/atl.h"

#include <vector>
#include <memory>

class ccl_worker;
class ccl_service_worker;
class ccl_master_sched;
class ccl_extra_sched;


class alignas(CACHELINE_SIZE) ccl_executor
{
public:
    ccl_executor(const ccl_executor& other) = delete;
    ccl_executor& operator=(const ccl_executor& other) = delete;
    ccl_executor(ccl_executor&& other) = delete;
    ccl_executor& operator=(ccl_executor&& other) = delete;

    ccl_executor();
    ~ccl_executor();

    void start(ccl_extra_sched* extra_sched);
    void start(ccl_master_sched* sched);
    void wait(const ccl_request* req);
    bool test(const ccl_request* req);

    size_t proc_idx{};
    size_t proc_count{};
    atl_desc_t* atl_desc = nullptr;

    bool is_tagged_coll_enabled = false;
    size_t tag_bits = 64;
    uint64_t max_tag = 0xFFFFFFFFFFFFFFFF;
    bool is_rma_enabled = false;
    size_t max_order_waw_size = 0;

private:
    void do_work(); 
    atl_comm_t** atl_comms = nullptr;
    std::vector<std::unique_ptr<ccl_worker>> workers;
};

template<class sched_type = ccl_master_sched>
inline void ccl_wait_impl(ccl_executor* exec, ccl_request* request)
{
    exec->wait(request);

    sched_type* sched = static_cast<sched_type*>(request);
    LOG_DEBUG("req ", request, " completed, sched ",
              ccl_coll_type_to_str(sched->coll_param.ctype));
    if (!sched->coll_attr.to_cache)
    {
        delete sched;
    }
    //*request = nullptr;
}

template<class sched_type = ccl_master_sched>
inline bool ccl_test_impl(ccl_executor* exec, ccl_request* request)
{
    bool completed = exec->test(request);

    if (completed)
    {
        sched_type* sched = static_cast<sched_type*>(request);
        LOG_DEBUG("req ", request, " completed, sched ",
                  ccl_coll_type_to_str(sched->coll_param.ctype));

        if (!sched->coll_attr.to_cache)
        {
            delete sched;
        }
    }

    return completed;
}
