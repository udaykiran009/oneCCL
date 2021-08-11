#include "sched/entry/gpu/ze_event_signal_entry.hpp"
#include "sched/queue/queue.hpp"
#include "common/utils/sycl_utils.hpp"

ze_event_signal_entry::ze_event_signal_entry(ccl_sched* sched, ccl_master_sched* master_sched)
        : sched_entry(sched),
          master_sched(master_sched) {
    CCL_THROW_IF_NOT(sched, "no sched");
    CCL_THROW_IF_NOT(master_sched, "no master_sched");
}

void ze_event_signal_entry::start() {
    LOG_DEBUG("signal event: ", master_sched->get_memory().sync_event);
    ZE_CALL(zeEventHostSignal, (master_sched->get_memory().sync_event));

    status = ccl_sched_entry_status_started;
}

void ze_event_signal_entry::update() {
    if (ccl::utils::is_sycl_event_completed(master_sched->get_native_event()) &&
        ccl::utils::is_sycl_event_completed(master_sched->get_sync_event())) {
        LOG_DEBUG("native and sync events are completed");
        status = ccl_sched_entry_status_complete;
    }
}
