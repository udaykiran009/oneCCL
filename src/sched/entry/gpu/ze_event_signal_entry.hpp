#pragma once

#include "oneapi/ccl/config.h"

#if defined(CCL_ENABLE_SYCL) && defined(MULTI_GPU_SUPPORT)

#include "sched/entry/entry.hpp"
#include "sched/sched.hpp"
#include "sched/master_sched.hpp"

class ze_event_signal_entry : public sched_entry {
public:
    static constexpr const char* class_name() noexcept {
        return "ZE_EVENT_SIGNAL";
    }

    ze_event_signal_entry() = delete;
    ze_event_signal_entry(ccl_sched* sched, ccl_master_sched* master_sched);
    ~ze_event_signal_entry() = default;

    void start() override;
    void update() override;

    const char* name() const override {
        return class_name();
    }

    bool is_strict_order_satisfied() override {
        return (status >= ccl_sched_entry_status_complete);
    }

private:
    ccl_master_sched* master_sched;
};

#endif