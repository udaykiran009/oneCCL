#include "common/global/global.hpp"
#include "exec/service_worker.hpp"

ccl_service_worker::ccl_service_worker(ccl_executor* executor, size_t idx,
                                       std::unique_ptr<ccl_sched_queue> data_queue,
                                       ccl_fusion_manager& fusion_manager)
    : ccl_worker(executor, idx, std::move(data_queue)),
      fusion_manager(fusion_manager)
{}


size_t ccl_service_worker::do_work()
{
    fusion_manager.execute();
    return ccl_worker::do_work();
}
