#pragma once

#include "atl/atl.h"
#include "coll/coll.hpp"
#include "common/env/env.hpp"
#include "common/global/global.hpp"
#include "common/request/request.hpp"
#include "exec/thread/listener.hpp"

#include <memory>
#include <vector>

class ccl_worker;
class ccl_service_worker;
class ccl_master_sched;
class ccl_extra_sched;


class alignas(CACHELINE_SIZE) ccl_executor
{
    friend class ccl_listener;
public:
    ccl_executor(const ccl_executor& other) = delete;
    ccl_executor& operator=(const ccl_executor& other) = delete;
    ccl_executor(ccl_executor&& other) = delete;
    ccl_executor& operator=(ccl_executor&& other) = delete;

    ccl_executor();
    ~ccl_executor();

    struct worker_guard
    {
        ccl_executor* parent;
        worker_guard(ccl_executor* e)
        {
            parent = e;
            parent->lock_workers();
        }
        worker_guard(worker_guard &) = delete;
        worker_guard(worker_guard&& src)
        {
            parent = src.parent;
            src.parent = nullptr;
        }
        ~worker_guard()
        {
            if (parent)
                parent->unlock_workers();
        }
    };

    worker_guard get_worker_lock() { return worker_guard(this); }


    void start(ccl_extra_sched* extra_sched);
    void start(ccl_master_sched* sched);
    void wait(const ccl_request* req);
    bool test(const ccl_request* req);
    size_t get_worker_count() const;

    ccl_status_t create_listener(ccl_resize_fn_t resize_func);
    void update_workers();
    void lock_workers();
    void unlock_workers();
    bool is_locked = false;

    size_t proc_idx{};
    size_t proc_count{};
    atl_desc_t* atl_desc = nullptr;

    bool is_tagged_coll_enabled = false;
    size_t tag_bits = 64;
    uint64_t max_tag = 0xFFFFFFFFFFFFFFFF;
    bool is_rma_enabled = false;
    size_t max_order_waw_size = 0;

private:
    size_t get_atl_comm_count(size_t worker_count);
    std::unique_ptr<ccl_sched_queue> create_sched_queue(size_t idx, size_t comm_per_worker);

    void do_work(); 
    atl_comm_t** atl_comms = nullptr;
    std::vector<std::unique_ptr<ccl_worker>> workers;
    std::unique_ptr<ccl_listener> listener;
};

template<class sched_type = ccl_master_sched>
inline void ccl_wait_impl(ccl_executor* exec, ccl_request* request)
{
    exec->wait(request);

    sched_type* sched = static_cast<sched_type*>(request);
    LOG_DEBUG("req ", request, " completed, sched ",
              ccl_coll_type_to_str(sched->coll_param.ctype));
    if (!sched->coll_attr.to_cache
        && !exec->is_locked
       )
    {
        delete sched;
    }
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

        if (!sched->coll_attr.to_cache
            && !exec->is_locked
           )
        {
            delete sched;
        }
    }

    return completed;
}
