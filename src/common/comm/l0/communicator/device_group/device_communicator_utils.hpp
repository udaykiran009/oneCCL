#pragma once
#include "common/comm/l0/devices/devices_declaration.hpp"
#include "common/comm/l0/device_community.hpp"

template <class buffer_type,
          ccl::group_split_type group_id,
          ccl::device_topology_type class_id,
          template <class, class, ccl::group_split_type>
          class algorithm>
struct communication_device_expander {
    template <class device_t, class... Args>
    void operator()(native::device_t_ptr<device_t>& comm_device,
                    std::shared_ptr<native::device_group_context>& ctx,
                    typename native::device_community_container<class_id>::element_type community,
                    Args&&... args) {
        if (comm_device) {
            LOG_DEBUG("Invoke: ", comm_device->to_string());

            using gpu_allreduce_entry = algorithm<buffer_type, device_t, group_id>;

            schedule =
                ctx->scheduler_impl
                    ->submit_entry<gpu_allreduce_entry, ccl_sched_add_back, group_id, class_id>(
                        *community, comm_device, std::forward<Args>(args)...);
        }
    }

    std::unique_ptr<ccl_gpu_sched> schedule;
};
