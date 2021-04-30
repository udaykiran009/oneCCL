#pragma once

#include "exec/thread/worker.hpp"
#include "fusion/fusion.hpp"
#include "internal_types.hpp"

class ccl_service_worker : public ccl_worker {
public:
    ccl_service_worker(size_t idx,
                       std::unique_ptr<ccl_sched_queue> data_queue,
                       ccl_fusion_manager& fusion_manager);
    ~ccl_service_worker();

    ccl::status do_work(size_t& processed_count) override;

    bool can_reset() override;

private:
    ccl_fusion_manager& fusion_manager;
};
