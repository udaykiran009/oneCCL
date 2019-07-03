#pragma once

#include "sched/sched_queue.hpp"

#include <memory>
#include <list>
#include <pthread.h>

class iccl_executor;

class iccl_worker
{
public:
    iccl_worker() = delete;
    iccl_worker(const iccl_worker& other) = delete;
    iccl_worker& operator= (const iccl_worker& other) = delete;
    iccl_worker(iccl_executor* executor, size_t idx, std::unique_ptr<iccl_sched_queue> queue);
    virtual ~iccl_worker() = default;

    iccl_status_t start();
    iccl_status_t stop();
    iccl_status_t pin(int proc_id);
    
    void add(iccl_sched* sched);

    virtual size_t do_work();

    size_t get_idx() { return idx; }

private:
    const size_t idx;
    pthread_t thread{};
    iccl_executor* executor = nullptr;
    std::unique_ptr<iccl_sched_queue> data_queue;
};
