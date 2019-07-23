#pragma once

#include "sched/sched_queue.hpp"

#include <memory>
#include <list>
#include <pthread.h>

class ccl_executor;

class ccl_worker
{
public:
    ccl_worker() = delete;
    ccl_worker(const ccl_worker& other) = delete;
    ccl_worker& operator= (const ccl_worker& other) = delete;
    ccl_worker(ccl_executor* executor, size_t idx, std::unique_ptr<ccl_sched_queue> queue);
    virtual ~ccl_worker() = default;

    ccl_status_t start();
    ccl_status_t stop();
    ccl_status_t pin(int proc_id);
    
    void add(ccl_sched* sched);

    virtual size_t do_work();

    size_t get_idx() { return idx; }

private:
    const size_t idx;
    pthread_t thread{};
    ccl_executor* executor = nullptr;
    std::unique_ptr<ccl_sched_queue> data_queue;
};
