#pragma once
#include "common/comm/l0/communicator/device_group/device_ring_communicator.hpp"
#include "common/comm/l0/communicator/typed_base_communicator_impl.hpp"

#include "common/comm/l0/devices/devices_declaration.hpp"
#include "common/comm/l0/device_community.hpp"
#include "common/comm/l0/context/device_group_ctx.hpp"
#include "common/comm/l0/scheduler/device_group_scheduler.hpp"

template<class buffer_type>
ccl::device_communicator::coll_request_t device_group_ring_communicator::allreduce_impl(const buffer_type* send_buf,
                                                                            buffer_type* recv_buf,
                                                                            size_t count,
                                                                            ccl::reduction reduction,
                                                                            const ccl::coll_attr* attr,
                                                                            ccl::stream::impl_t& stream)
{
    using namespace native;

    static constexpr ccl::device_topology_type topology = base_t::topology_type();
    if(!is_ready())
    {
        throw ccl::ccl_error(std::string("Device communicator for topology: " + ::to_string(topology) +
                                         " is not ready yet. Not all сommunicators are created in group. Please create them before usage"));
    }
    size_t comm_rank = rank();
    LOG_DEBUG("device_communicator for device idx: ", get_device_path(),
              ", rank idx: ", comm_rank);

    //TODO make const!
    ccl_buffer send_entry_buffer(const_cast<buffer_type**>(&send_buf), count * sizeof(buffer_type), 0, ccl_buffer_type::INDIRECT);
    ccl_buffer recv_entry_buffer(&recv_buf, count * sizeof(buffer_type), 0, ccl_buffer_type::INDIRECT);


    const auto &in_process_gpu_storage = device_community_impl->get_devices<ccl_gpu_comm>();
    const auto &virtual_process_gpu_storage = device_community_impl->get_devices<ccl_virtual_gpu_comm>();

    device_group_scheduler::schedule_ptr schedule;

    //source for collective operation is real gpu or virtual gpu
    auto real_device_it = in_process_gpu_storage.find(comm_rank);
    if(real_device_it != in_process_gpu_storage.end())
    {
        LOG_DEBUG("Invoke: ", real_device_it->second->to_string());
        using gpu_allreduce_entry = l0_allreduce_typed_entry<buffer_type, ccl_gpu_comm, topology>;

        schedule =
            ctx->scheduler_impl->submit_entry<gpu_allreduce_entry, ccl_sched_add_back>(*device_community_impl,
                                                                                       real_device_it->second,send_entry_buffer,
                                                                                       recv_entry_buffer,
                                                                                       count,
                                                                                       static_cast<ccl_reduction_t>(reduction),
                                                                                       stream);
    }
    else
    {

    auto virtual_device_it = virtual_process_gpu_storage.find(comm_rank);
    if(virtual_device_it != virtual_process_gpu_storage.end())
    {
        LOG_DEBUG("Invoke: ", virtual_device_it->second->to_string());
        using gpu_allreduce_entry = l0_allreduce_typed_entry<buffer_type, ccl_virtual_gpu_comm, topology>;

        schedule =
            ctx->scheduler_impl->submit_entry<gpu_allreduce_entry, ccl_sched_add_back>(*device_community_impl,
                                                                                       virtual_device_it->second,send_entry_buffer,
                                                                                       recv_entry_buffer,
                                                                                       count,
                                                                                       static_cast<ccl_reduction_t>(reduction),
                                                                                       stream);
    }}

    //if sched is not ready - send NULL
    if(schedule)
    {
        LOG_DEBUG("Device group finalized");
    }
    return std::unique_ptr<gpu_request_impl>(new gpu_request_impl(std::move(schedule)));
}
