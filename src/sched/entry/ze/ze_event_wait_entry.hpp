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

    explicit ze_event_wait_entry(ccl_sched* sched, std::vector<ze_event_handle_t> wait_events);

    void start() override;
    void update() override;

private:
    std::list<ze_event_handle_t> wait_events;

    bool check_event_status(ze_event_handle_t event) const;
};
