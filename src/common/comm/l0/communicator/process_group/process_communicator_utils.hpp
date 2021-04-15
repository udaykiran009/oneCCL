#pragma once
#include "common/comm/l0/devices/devices_declaration.hpp"
#include "common/comm/l0/device_community.hpp"

template <ccl::group_split_type group_id,
          ccl::device_topology_type class_id,
          template <class, ccl::group_split_type>
          class algorithm>
struct communication_process_device_expander {
    template <class device_t, class... Args>
    void operator()(native::device_t_ptr<device_t>& comm_device,
                    std::shared_ptr<native::process_group_context>& ctx,
                    typename native::device_community_container<class_id>::element_type community,
                    size_t process_id,
                    size_t thread_id,
                    Args&&... args) {
        if (comm_device) {
            LOG_DEBUG("Invoke: ", comm_device->to_string());

            using gpu_entry = algorithm<device_t, group_id>;

            schedule = ctx->scheduler_impl
                           ->submit_entry<gpu_entry, ccl_sched_add_back, group_id, class_id>(
                               process_id,
                               thread_id,
                               *community,
                               comm_device,
                               std::forward<Args>(args)...);
        }
    }

    std::shared_ptr<ccl_gpu_sched> schedule;
};

template <ccl::group_split_type group_id,
          ccl::device_topology_type class_id,
          template <class, ccl::group_split_type>
          class algorithm,
          class... Args>
std::unique_ptr<ccl::event_impl> do_collective_op(
    // TODO: can we avoid using device_variant here? Because it creates an instantiation of entry for each device which
    // makes it slow to compile
    native::device_variant_t<native::ccl_gpu_comm,
                             native::ccl_virtual_gpu_comm,
                             native::ccl_ipc_source_gpu_comm<native::ccl_gpu_comm>,
                             native::ccl_ipc_source_gpu_comm<native::ccl_virtual_gpu_comm>,
                             native::ccl_numa_proxy<native::ccl_gpu_comm>,
                             native::ccl_numa_proxy<native::ccl_virtual_gpu_comm>,
                             native::ccl_scaleout_proxy<native::ccl_gpu_comm>,
                             native::ccl_scaleout_proxy<native::ccl_virtual_gpu_comm>>&
        communication_device,
    std::shared_ptr<native::process_group_context>& ctx,
    typename native::device_community_container<class_id>::element_type community,
    size_t process_id,
    size_t thread_id,
    native::ccl_driver_context_ptr native_context,
    Args&&... args) {
    communication_process_device_expander<group_id, class_id, algorithm> expander;
    ccl_tuple_for_each_args(communication_device,
                            expander,
                            ctx,
                            community,
                            process_id,
                            thread_id,
                            native_context,
                            std::forward<Args>(args)...);
    if (expander.schedule) {
        LOG_DEBUG("Device group finalized");
    }
    return std::unique_ptr<ccl::event_impl>(
        new ccl::gpu_shared_event_impl(std::move(expander.schedule)));
}