#include "exec/thread/worker.hpp"

static void* ccl_worker_func(void* args);

ccl_worker::ccl_worker(size_t idx, std::unique_ptr<ccl_sched_queue> queue)
        : ccl_base_thread(idx, ccl_worker_func),
          should_lock(false),
          is_locked(false),
          strict_sched_queue(std::unique_ptr<ccl_strict_sched_queue>(new ccl_strict_sched_queue())),
          sched_queue(std::move(queue)) {}

void ccl_worker::add(ccl_sched* sched) {}

ccl::status ccl_worker::do_work(size_t& processed_count) {
    return ccl::status::success;
}

ccl::status ccl_worker::process_strict_sched_queue() {
    return ccl::status::success;
}

ccl::status ccl_worker::process_sched_queue(size_t& completed_sched_count, bool process_all) {
    return ccl::status::success;
}

ccl::status ccl_worker::process_sched_bin(ccl_sched_bin* bin, size_t& completed_sched_count) {
    return ccl::status::success;
}

void ccl_worker::clear_queue() {}

static inline bool ccl_worker_check_conditions(ccl_worker* worker,
                                               size_t iter_count,
                                               int do_work_status) {
    return true;
}

static void* ccl_worker_func(void* args) {
    return nullptr;
}
