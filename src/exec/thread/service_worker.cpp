#include "common/global/global.hpp"
#include "exec/thread/service_worker.hpp"

ccl_service_worker::ccl_service_worker(ccl_executor* executor, size_t idx,
                                       std::unique_ptr<ccl_sched_queue> data_queue,
                                       ccl_fusion_manager& fusion_manager)
    : ccl_worker(executor, idx, std::move(data_queue)),
      fusion_manager(fusion_manager)
{}


ccl_status_t ccl_service_worker::do_work(size_t& processed_count)
{
    fusion_manager.execute();
    return ccl_worker::do_work(processed_count);
}
