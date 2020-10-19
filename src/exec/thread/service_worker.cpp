#include "exec/thread/service_worker.hpp"

ccl_service_worker::ccl_service_worker(size_t idx,
                                       std::unique_ptr<ccl_sched_queue> data_queue,
                                       ccl_fusion_manager& fusion_manager)
        : ccl_worker(idx, std::move(data_queue)),
          fusion_manager(fusion_manager) {}

ccl::status ccl_service_worker::do_work(size_t& processed_count) {
    fusion_manager.execute();
    return ccl_worker::do_work(processed_count);
}
