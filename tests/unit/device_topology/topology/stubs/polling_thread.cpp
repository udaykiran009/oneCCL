#include "common/comm/l0/context/scale/polling_thread.hpp"

namespace native {
polling_thread::polling_thread(size_t idx, std::unique_ptr<ccl_sched_queue> queue)
        : ccl_worker(idx, std::move(queue)),
          numa_node(idx) {}

ccl::status polling_thread::do_work(size_t& processed_count) {
    return ccl::status::success;
}
} // namespace native
