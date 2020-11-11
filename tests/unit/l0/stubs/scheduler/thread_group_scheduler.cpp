#if 0
#pragma once
#include "common/utils/spinlock.hpp"
#include "sched/gpu_concurrent_sched.hpp"
#include "common/comm/l0/device_community.hpp"

namespace native {

//template<ccl_coll_type entry_type>
struct thread_group_scheduler {
    using schedule_ptr = std::unique_ptr<ccl_gpu_concurrent_sched>;
    using thread_schedule_ptr = std::shared_ptr<ccl_gpu_sched>;

    thread_group_scheduler(size_t threads_count) : thread_group_size(threads_count) {
    }

    template <class EntryType,
              ccl_sched_add_mode mode,
              ccl::group_split_type group_id,
              ccl::device_topology_type class_id,
              class device_t,
              class... Arguments>
    thread_schedule_ptr submit_entry(size_t thread_id,
                                     device_community<class_id>& device_topology,
                                     device_t& device,
                                     native::ccl_driver_context_ptr ctx,
                                     Arguments&&... args) {
        return thread_schedule_ptr{};
    }

protected:
    size_t thread_group_size;
};

} // namespace native
#endif
