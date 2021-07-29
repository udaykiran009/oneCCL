#include "sched/queue/queue.hpp"
#include "ze_event_signal_entry.hpp"
#include "common/utils/sycl_utils.hpp"

#if defined(CCL_ENABLE_SYCL) && defined(MULTI_GPU_SUPPORT)
ze_event_signal_entry::ze_event_signal_entry(ccl_sched* sched, ccl_master_sched* master_sched)
        : sched_entry{ sched },
          master_sched{ master_sched } {}

void ze_event_signal_entry::start() {
    LOG_DEBUG("signal event: ", master_sched->get_memory().sync_event);
    ZE_CALL(zeEventHostSignal, (master_sched->get_memory().sync_event));

    status = ccl_sched_entry_status_started;
}

void ze_event_signal_entry::update() {
    if (utils::is_sycl_event_completed(master_sched->get_native_event()) &&
        utils::is_sycl_event_completed(master_sched->get_sync_event())) {
        LOG_DEBUG("native and sync events are completed");
        status = ccl_sched_entry_status_complete;
    }
}
#endif