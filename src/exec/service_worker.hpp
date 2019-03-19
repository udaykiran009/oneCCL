#pragma once

#include "exec/worker.hpp"
#include "fusion/fusion.hpp"

class mlsl_service_worker : public mlsl_worker
{
public:
    mlsl_service_worker(mlsl_executor* executor, size_t idx, std::unique_ptr<mlsl_sched_queue> data_queue,
                        mlsl_fusion_manager& fusion_manager);
    ~mlsl_service_worker() = default;

    size_t do_work();

private:
    mlsl_fusion_manager& fusion_manager;
};
