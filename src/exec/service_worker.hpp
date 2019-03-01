#pragma once

#include "exec/worker.hpp"

class mlsl_service_worker : public mlsl_worker
{
public:
    mlsl_service_worker(mlsl_executor* executor, size_t idx, std::unique_ptr<mlsl_sched_queue> data_queue,
                        std::unique_ptr<mlsl_sched_queue> service_queue);
    ~mlsl_service_worker();

    void add(mlsl_sched* sched);
    size_t do_work();

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
        persistent_service_scheds.emplace_back(sched);
    }
    void register_temporal_service_sched(mlsl_sched* sched)
    {
        MLSL_ASSERT(sched);
        temporal_service_scheds.emplace_back(sched);
    }
    void check_persistent();
    void check_temporal();
    void erase_service_scheds(std::list<std::shared_ptr<mlsl_sched>>& scheds);
};
