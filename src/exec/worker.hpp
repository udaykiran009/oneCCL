#pragma once

#include "exec/exec.hpp"
#include "sched/sched_queue.hpp"
#include <memory>
#include <list>
#include <pthread.h>

class mlsl_worker
{
public:
    mlsl_worker() = delete;
    mlsl_worker(const mlsl_worker& other) = delete;
    mlsl_worker& operator= (const mlsl_worker& other) = delete;
    mlsl_worker(mlsl_executor* executor, size_t idx, std::unique_ptr<mlsl_sched_queue> queue);
    virtual ~mlsl_worker() = default;

    mlsl_status_t start();
    mlsl_status_t stop();
    mlsl_status_t pin(int proc_id);
    
    virtual void add(mlsl_sched* sched);

    virtual size_t do_work();

    size_t get_idx() { return idx; }

private:
    const size_t idx;
    pthread_t thread{};
    mlsl_executor* executor = nullptr;
    std::unique_ptr<mlsl_sched_queue> data_queue;
};
