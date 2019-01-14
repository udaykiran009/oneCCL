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
    mlsl_status_t pin_to_proc(int proc_id);
    void stop();
    virtual void add_to_queue(mlsl_sched* sched);
    virtual size_t peek_and_progress();
    const size_t idx;

private:
    pthread_t thread{};

    mlsl_executor* executor = nullptr;
    std::unique_ptr<mlsl_sched_queue> data_queue;
};

class mlsl_service_worker : public mlsl_worker
{
public:
    mlsl_service_worker(mlsl_executor* executor, size_t idx, std::unique_ptr<mlsl_sched_queue> data_queue,
                        std::unique_ptr<mlsl_sched_queue> service_queue);
    void add_to_queue(mlsl_sched* sched);
    size_t peek_and_progress();
    ~mlsl_service_worker();

private:
    std::unique_ptr<mlsl_sched_queue> service_queue{};
    //todo: replace with unique_ptr when sched has a proper destructor
    std::list<std::shared_ptr<mlsl_sched>> temporal_service_scheds{};
    std::list<std::shared_ptr<mlsl_sched>> persistent_service_scheds{};
    size_t service_queue_skip_count{};

    void peek_service();
    void register_persistent_service_sched(mlsl_sched* sched)
    {
        MLSL_ASSERT(sched);
        persistent_service_scheds.emplace_back(sched, mlsl_sched_free);
    }
    void register_temporal_service_sched(mlsl_sched* sched)
    {
        MLSL_ASSERT(sched);
        temporal_service_scheds.emplace_back(sched, mlsl_sched_free);
    }
    void check_persistent();
    void check_temporal();
    void erase_service_scheds(std::list<std::shared_ptr<mlsl_sched>>& scheds);
};
