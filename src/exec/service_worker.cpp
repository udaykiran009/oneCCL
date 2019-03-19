#include "common/global/global.hpp"
#include "exec/service_worker.hpp"

mlsl_service_worker::mlsl_service_worker(mlsl_executor* executor, size_t idx,
                                         std::unique_ptr<mlsl_sched_queue> data_queue,
                                         mlsl_fusion_manager& fusion_manager)
    : mlsl_worker(executor, idx, std::move(data_queue)),
      fusion_manager(fusion_manager)
{}


size_t mlsl_service_worker::do_work()
{
    fusion_manager.execute();

    return mlsl_worker::do_work();
}
