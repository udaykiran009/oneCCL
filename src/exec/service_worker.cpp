#include "common/global/global.hpp"
#include "exec/service_worker.hpp"

iccl_service_worker::iccl_service_worker(iccl_executor* executor, size_t idx,
                                         std::unique_ptr<iccl_sched_queue> data_queue,
                                         iccl_fusion_manager& fusion_manager)
    : iccl_worker(executor, idx, std::move(data_queue)),
      fusion_manager(fusion_manager)
{}


size_t iccl_service_worker::do_work()
{
    fusion_manager.execute();
    return iccl_worker::do_work();
}
