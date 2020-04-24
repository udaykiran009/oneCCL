#pragma once
#include "sched/gpu_sched.hpp"
#include "sched/entry/l0/l0_allreduce_typed_entry.hpp"
//#include "sched/entry/l0/l0_allgather_handles_entry.hpp"
#include "sched/entry/factory/entry_factory.hpp"

#include "common/comm/l0/device_community.hpp"
namespace native
{

//template<ccl_coll_type entry_type>
struct device_group_scheduler
{
    using schedule_ptr = std::unique_ptr<ccl_gpu_sched>;
    //create schedule
  /*
     static constexpr ccl_coll_type type() noexcept
    {
        return entry_type;
    }
  */
    template<class EntryType, ccl_sched_add_mode mode,
            ccl::device_topology_type topology,
            class device_t, class ...Arguments>
    schedule_ptr submit_entry(device_community<topology>& device_topology, device_t& device, Arguments &&...args)
    {
        //create schedule
        size_t group_size = device_topology.template get_device_count<native::ccl_gpu_comm>()  +
                            device_topology.template get_device_count<native::ccl_virtual_gpu_comm>();

        //make entry
        if (!current_schedule)
        {
            current_schedule.reset(new ccl_gpu_sched(device_topology.get_device_storage(), group_size));
        }

        auto created_entry =
            entry_factory::make_ordered_entry<EntryType,
                                              mode>(current_schedule.get(),
                                                    device,
                                                    device_topology.get_device_storage(),
                                                    std::forward<Arguments>(args)...);
        LOG_DEBUG("do initial progress");

        created_entry->start();
        current_schedule->set_fence(created_entry->get_fence()); //TODO temporary

        //active_group_sched->add_entry(std::move(created_entry));
        current_schedule->do_progress();


        LOG_DEBUG("Device group filled for: ", current_schedule->entries_count(), "/", group_size);
        if(current_schedule->entries_count() == group_size)
        {
            LOG_DEBUG("Device group finalized");
            schedule_ptr ret(new ccl_gpu_sched(device_topology.get_device_storage(), group_size));
            ret.swap(current_schedule);

            return ret;

        }
        //if sched is not ready - send NULL
        return std::unique_ptr<ccl_gpu_sched>();

    }
private:
    schedule_ptr current_schedule;
};

}
