#pragma once

#include "exec/worker.hpp"
#include "fusion/fusion.hpp"

class ccl_service_worker : public ccl_worker
{
public:
    ccl_service_worker(ccl_executor* executor, size_t idx, std::unique_ptr<ccl_sched_queue> data_queue,
                       ccl_fusion_manager& fusion_manager);
    ~ccl_service_worker() = default;

    size_t do_work();

private:
    ccl_fusion_manager& fusion_manager;
};
