#pragma once

#include "sched/entry/entry.hpp"
#include "sched/master_sched.hpp"
#include "sched/sched.hpp"

class ze_event_signal_entry : public sched_entry {
public:
    static constexpr const char* class_name() noexcept {
        return "ZE_EVENT_SIGNAL";
    }

    const char* name() const override {
        return class_name();
    }

    bool is_strict_order_satisfied() override {
        return (status >= ccl_sched_entry_status_complete);
    }

    ze_event_signal_entry() = delete;
    explicit ze_event_signal_entry(ccl_sched* sched, ccl_master_sched* master_sched);
    ze_event_signal_entry(const ze_event_signal_entry&) = delete;

    void start() override;
    void update() override;

private:
    ccl_master_sched* const master_sched;
};
