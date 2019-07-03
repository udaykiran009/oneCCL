#pragma once

#include "exec/worker.hpp"
#include "fusion/fusion.hpp"

class iccl_service_worker : public iccl_worker
{
public:
    iccl_service_worker(iccl_executor* executor, size_t idx, std::unique_ptr<iccl_sched_queue> data_queue,
                        iccl_fusion_manager& fusion_manager);
    ~iccl_service_worker() = default;

    size_t do_work();

private:
    iccl_fusion_manager& fusion_manager;
};
