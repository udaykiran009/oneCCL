#pragma once
#include "common/comm/l0/devices/devices_declaration.hpp"
#include "common/comm/l0/device_community.hpp"

template <ccl::group_split_type group_id,
          ccl::device_topology_type class_id,
          template <class, ccl::group_split_type>
          class algorithm>
struct communication_device_expander {
    template <class device_t, class... Args>
    void operator()(native::device_t_ptr<device_t>& comm_device,
                    std::shared_ptr<native::device_group_context>& ctx,
                    typename native::device_community_container<class_id>::element_type community,
                    Args&&... args) {
        if (comm_device) {
            LOG_DEBUG("Invoke: ", comm_device->to_string());

            using gpu_entry = algorithm<device_t, group_id>;

            schedule = ctx->scheduler_impl
                           ->submit_entry<gpu_entry, ccl_sched_add_back, group_id, class_id>(
                               *community, comm_device, std::forward<Args>(args)...);
        }
    }

    std::unique_ptr<ccl_gpu_sched> schedule;
};

template <ccl::group_split_type group_id,
          ccl::device_topology_type class_id,
          template <class, ccl::group_split_type>
          class algorithm,
          class... Args>
std::unique_ptr<ccl::event_impl> do_collective_op(
    native::device_variant_t<native::ccl_gpu_comm, native::ccl_virtual_gpu_comm>&
        communication_device,
    std::shared_ptr<native::device_group_context>& ctx,
    typename native::device_community_container<class_id>::element_type community,
    native::ccl_driver_context_ptr native_context,
    Args&&... args) {
    communication_device_expander<group_id, class_id, algorithm> expander;
    ccl_tuple_for_each_args(communication_device,
                            expander,
                            ctx,
                            community,
                            native_context,
                            std::forward<Args>(args)...);
    if (expander.schedule) {
        LOG_DEBUG("Device group finalized");
    }
    return std::unique_ptr<ccl::event_impl>(
        new ccl::gpu_shared_event_impl(std::move(expander.schedule)));
}