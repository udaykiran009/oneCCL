#pragma once
#include "common/utils/spinlock.hpp"
#include "sched/gpu_concurrent_sched.hpp"
#include "sched/entry/l0/l0_allreduce_typed_entry.hpp"
//#include "sched/entry/l0/l0_allgather_handles_entry.hpp"
#include "sched/entry/factory/entry_factory.hpp"
#include "common/comm/l0/device_community.hpp"

namespace native {

//template<ccl_coll_type entry_type>
struct thread_group_scheduler {
    using schedule_ptr = std::unique_ptr<ccl_gpu_concurrent_sched>;
    using thread_schedule_ptr = std::shared_ptr<ccl_gpu_sched>;
    //create schedule
    /*
     static constexpr ccl_coll_type type() noexcept
    {
        return entry_type;
    }
  */
    thread_group_scheduler(size_t threads_count) : thread_group_size(threads_count) {
        //make concurrent chedule
        if (!current_schedule) {
            current_schedule = ccl_gpu_concurrent_sched::create(thread_group_size);
        }
    }

    template <class EntryType,
              ccl_sched_add_mode mode,
              ccl::device_group_split_type group_id,
              ccl::device_topology_type class_id,
              class device_t,
              class... Arguments>
    thread_schedule_ptr submit_entry(size_t thread_id,
                                     device_community<class_id>& device_topology,
                                     device_t& device,
                                     Arguments&&... args) {
        const topology_addr<group_id, class_id>& comm_data =
            device->template get_comm_data<group_id, class_id>();
        size_t device_group_size =
            device_topology.template get_device_count<native::ccl_gpu_comm>() +
            device_topology.template get_device_count<native::ccl_virtual_gpu_comm>();

        LOG_DEBUG("Thread id: ",
                  thread_id,
                  " device_group_size: ",
                  device_group_size,
                  " comm data: ",
                  comm_data.to_string());
        //get thread local schedule
        auto current_thread_schedule = current_schedule->get_gpu_sched(thread_id);
        if (!current_thread_schedule) {
            current_thread_schedule = current_schedule->create_gpu_sched(
                thread_id, device_topology.get_device_storage(), comm_data.size);
        }

        // create entry
        auto created_entry =
            entry_factory::make_ordered_entry<EntryType, mode>(current_thread_schedule.get(),
                                                               device,
                                                               device_topology.get_device_storage(),
                                                               std::forward<Arguments>(args)...);
        LOG_DEBUG("do initial entry progress");
        created_entry->start();
        current_thread_schedule->set_fence(created_entry->get_fence()); //TODO temporary
        current_thread_schedule->do_progress();

        LOG_DEBUG("Device group filled for: ",
                  current_thread_schedule->entries_count(),
                  "/",
                  device_group_size);
        if (current_thread_schedule->entries_count() == device_group_size) {
            LOG_DEBUG("Device group finalized");
            current_schedule->create_gpu_sched(
                thread_id, device_topology.get_device_storage(), comm_data.size);
            ;
            return current_thread_schedule;
        }
        //if sched is not ready - send NULL
        return thread_schedule_ptr();
    }

protected:
    schedule_ptr current_schedule;
    size_t thread_group_size;
};

} // namespace native
