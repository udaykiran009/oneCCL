#pragma once

#include "sched/entry/entry.hpp"
#include "sched/sched.hpp"

class ccl_master_sched;

class ze_event_signal_entry : public sched_entry {
public:
    static constexpr const char* class_name() noexcept {
        return "ZE_EVENT_SIGNAL";
    }

    const char* name() const override {
        return class_name();
    }

    ze_event_signal_entry() = delete;
    explicit ze_event_signal_entry(ccl_sched* sched, ccl_master_sched* master_sched);
    explicit ze_event_signal_entry(ccl_sched* sched, ze_event_handle_t event);
    ze_event_signal_entry(const ze_event_signal_entry&) = delete;

    void start() override;
    void update() override;

private:
    ccl_master_sched* const master_sched{};
    const ze_event_handle_t event{};

    void handle_sycl_event_status();
};
