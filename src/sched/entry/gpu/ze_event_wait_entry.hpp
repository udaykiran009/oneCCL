#pragma once

#include "sched/entry/entry.hpp"
#include "sched/sched.hpp"

class ze_event_wait_entry : public sched_entry {
public:
    static constexpr const char* class_name() noexcept {
        return "ZE_EVENT_WAIT";
    }

    const char* name() const override {
        return class_name();
    }

    bool is_gpu_entry() const noexcept override {
        return true;
    }

    bool is_strict_order_satisfied() override {
        return (status >= ccl_sched_entry_status_complete);
    }

    explicit ze_event_wait_entry(ccl_sched* sched, ze_event_handle_t event);

    void start() override;
    void update() override;

private:
    const ze_event_handle_t event;

    void check_event_status();
};
