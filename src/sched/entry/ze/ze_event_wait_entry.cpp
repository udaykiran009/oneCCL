#include "sched/entry/ze/ze_event_wait_entry.hpp"
#include "sched/entry/ze/ze_primitives.hpp"

ze_event_wait_entry::ze_event_wait_entry(ccl_sched* sched,
                                         std::vector<ze_event_handle_t> wait_events)
        : sched_entry(sched),
          wait_events(wait_events.cbegin(), wait_events.cend()) {
    CCL_THROW_IF_NOT(sched, "no sched");
}

bool ze_event_wait_entry::check_event_status(ze_event_handle_t event) const {
    auto query_status = zeEventQueryStatus(event);
    if (query_status == ZE_RESULT_SUCCESS) {
        return true;
    }
    else if (query_status != ZE_RESULT_NOT_READY) {
        CCL_THROW("ze error at zeEventQueryStatus, code: ", ccl::ze::to_string(query_status));
    }

    return false;
}

void ze_event_wait_entry::start() {
    LOG_DEBUG("start event waiting");
    status = ccl_sched_entry_status_started;
}

void ze_event_wait_entry::update() {
    for (auto it = wait_events.begin(); it != wait_events.end();) {
        bool is_completed = check_event_status(*it);
        if (!is_completed) {
            return;
        }
        it = wait_events.erase(it);
    }
    status = ccl_sched_entry_status_complete;
}
