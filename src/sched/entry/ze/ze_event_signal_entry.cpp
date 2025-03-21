#include "common/utils/sycl_utils.hpp"
#include "sched/entry/ze/ze_event_signal_entry.hpp"
#include "sched/queue/queue.hpp"
#include "sched/sched.hpp"

ze_event_signal_entry::ze_event_signal_entry(ccl_sched* sched, ccl_sched* master_sched)
        : sched_entry(sched),
          master_sched(master_sched) {
    CCL_THROW_IF_NOT(sched, "no sched");
    CCL_THROW_IF_NOT(master_sched, "no master_sched");
}

ze_event_signal_entry::ze_event_signal_entry(ccl_sched* sched, ze_event_handle_t event)
        : sched_entry(sched),
          event(event) {
    CCL_THROW_IF_NOT(sched, "no sched");
}

void ze_event_signal_entry::start() {
    auto signal_event =
        (master_sched) ? ccl::utils::get_native_event(master_sched->get_request()->get_sync_event())
                       : event;

    LOG_DEBUG("signal event: ", signal_event);
    ZE_CALL(zeEventHostSignal, (signal_event));

    status = ccl_sched_entry_status_started;
}

void ze_event_signal_entry::handle_sycl_event_status() {
    if (ccl::utils::is_sycl_event_completed(master_sched->get_request()->get_native_event()) &&
        ccl::utils::is_sycl_event_completed(master_sched->get_request()->get_sync_event())) {
        LOG_DEBUG("native and sync events are completed");
        status = ccl_sched_entry_status_complete;
    }
}

void ze_event_signal_entry::update() {
    if (master_sched) {
        handle_sycl_event_status();
    }
    else {
        status = ccl_sched_entry_status_complete;
    }
}
