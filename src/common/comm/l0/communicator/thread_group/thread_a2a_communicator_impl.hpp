#pragma once
#include "common/comm/l0/communicator/thread_group/thread_a2a_communicator.hpp"
#include "common/comm/l0/communicator/typed_base_communicator_impl.hpp"

#include "common/comm/l0/devices/devices_declaration.hpp"
#include "common/comm/l0/device_community.hpp"
#include "common/comm/l0/context/thread_group_ctx.hpp"
#include "common/comm/l0/scheduler/thread_group_scheduler.hpp"
#include "common/request/gpu_request.hpp"

/* allgatherv */
template<class buffer_type>
ccl::communicator::coll_request_t
thread_device_group_a2a_communicator::allgatherv_impl(const buffer_type& send_buf,
                                                      size_t send_count,
                                                      buffer_type& recv_buf,
                                                      const size_t* recv_counts,
                                                      const ccl::coll_attr* attr,
                                                      ccl::stream::impl_t& stream)
{
    throw ccl::ccl_error(std::string(__PRETTY_FUNCTION__) + " - is not implemented");
    return {};
}

template<class buffer_type>
ccl::communicator::coll_request_t
thread_device_group_a2a_communicator::allgatherv_impl(const buffer_type* send_buf,
                                                      size_t send_count,
                                                      buffer_type* recv_buf,
                                                      const size_t* recv_counts,
                                                      const ccl::coll_attr* attr,
                                                      ccl::stream::impl_t& stream)
{
    throw ccl::ccl_error(std::string(__PRETTY_FUNCTION__) + " - is not implemented");
    return {};
}


/* allreduce */
template<class buffer_type>
ccl::communicator::coll_request_t
thread_device_group_a2a_communicator::allreduce_impl(const buffer_type& send_buf,
                                                     buffer_type& recv_buf,
                                                     size_t count,
                                                     ccl::reduction reduction,
                                                     const ccl::coll_attr* attr,
                                                     ccl::stream::impl_t& stream)
{
    throw ccl::ccl_error(std::string(__PRETTY_FUNCTION__) + " - is not implemented");
    return {};
}

template<class buffer_type>
ccl::communicator::coll_request_t
thread_device_group_a2a_communicator::allreduce_impl(const buffer_type* send_buf,
                                                     buffer_type* recv_buf,
                                                     size_t count,
                                                     ccl::reduction reduction,
                                                     const ccl::coll_attr* attr,
                                                     ccl::stream::impl_t& stream)
{
    using namespace native;

    static constexpr ccl::device_topology_type topology =  base_t::topology_type();

    if(!is_ready())
    {
        throw ccl::ccl_error(std::string("Device communicator for topology: " + ::to_string(topology) +
                                         " is not ready yet. Not all сommunicators are created in group. Please create them before usage"));
    }

    size_t comm_rank = rank();
    LOG_DEBUG("communicator for device idx: ", get_device_path(),
              ", rank idx: ", comm_rank);

    //TODO make const!
    ccl_buffer send_entry_buffer(const_cast<buffer_type**>(&send_buf), count * sizeof(buffer_type), 0, ccl_buffer_type::INDIRECT);
    ccl_buffer recv_entry_buffer(&recv_buf, count * sizeof(buffer_type), 0, ccl_buffer_type::INDIRECT);

    const auto &in_process_gpu_storage = device_community_impl->get_devices<ccl_gpu_comm>();
    const auto &virtual_process_gpu_storage = device_community_impl->get_devices<ccl_virtual_gpu_comm>();
    auto &ipc_gpu_storage = device_community_impl->get_devices<ccl_ipc_gpu_comm>();
    (void)ipc_gpu_storage;

    thread_group_scheduler::thread_schedule_ptr schedule;
    //source for collective operation is real gpu or virtual gpu
    auto real_device_it = in_process_gpu_storage.find(comm_rank);
    if(real_device_it != in_process_gpu_storage.end())
    {
        LOG_DEBUG("Invoke: ", real_device_it->second->to_string());
        /*
        using gpu_allreduce_entry = l0_allreduce_typed_entry<buffer_type, ccl_gpu_comm, topology>;

        schedule =
                ctx->scheduler_impl->submit_entry<gpu_allreduce_entry, ccl_sched_add_back>(thread_id,
                                                                                           *ctx->get_thread_topology<thread_device_group_ring_communicator::topology_class()>(thread_id),
                                                                                           real_device_it->second,send_entry_buffer,
                                                                                           recv_entry_buffer,
                                                                                           count,
                                                                                           static_cast<ccl_reduction_t>(reduction));
        */
    }
    else
    {
    auto virtual_device_it = virtual_process_gpu_storage.find(comm_rank);
    if(virtual_device_it != virtual_process_gpu_storage.end())
    {
        LOG_DEBUG("Invoke: ", virtual_device_it->second->to_string());
        /*
        using gpu_allreduce_entry = l0_allreduce_typed_entry<buffer_type, ccl_virtual_gpu_comm, topology>;


        schedule =
            ctx->scheduler_impl->submit_entry<gpu_allreduce_entry, ccl_sched_add_back>(thread_id,
                                                                                       *ctx->get_thread_topology<thread_device_group_ring_communicator::topology_class()>(thread_id),
                                                                                       virtual_device_it->second, send_entry_buffer,
                                                                                       recv_entry_buffer,
                                                                                       count,
                                                                                       static_cast<ccl_reduction_t>(reduction));
        */
    }}

    //if sched is not ready - send NULL
    if(schedule)
    {
        LOG_DEBUG("Device group finalized");
    }
    return std::unique_ptr<ccl::gpu_shared_request_impl>(new ccl::gpu_shared_request_impl(std::move(schedule)));
}


/* alltoall */
template<class buffer_type>
ccl::communicator::coll_request_t
thread_device_group_a2a_communicator::alltoall_impl(const buffer_type* send_buf,
                                                    buffer_type* recv_buf,
                                                    size_t count,
                                                    const ccl::coll_attr* attr,
                                                    ccl::stream::impl_t& stream)
{
    throw ccl::ccl_error(std::string(__PRETTY_FUNCTION__) + " - is not implemented");
    return {};
}

template<class buffer_type>
ccl::communicator::coll_request_t
thread_device_group_a2a_communicator::alltoall_impl(const buffer_type& send_buf,
                                                    buffer_type& recv_buf,
                                                    size_t count,
                                                    const ccl::coll_attr* attr,
                                                    ccl::stream::impl_t& stream)
{
    throw ccl::ccl_error(std::string(__PRETTY_FUNCTION__) + " - is not implemented");
    return {};
}


/* alltoallv */
template<class buffer_type>
ccl::communicator::coll_request_t
thread_device_group_a2a_communicator::alltoallv_impl(const buffer_type* send_buf,
                                                     const size_t* send_counts,
                                                     buffer_type* recv_buf,
                                                     const size_t* recv_counts,
                                                     const ccl::coll_attr* attr,
                                                     ccl::stream::impl_t& stream)
{
    throw ccl::ccl_error(std::string(__PRETTY_FUNCTION__) + " - is not implemented");
    return {};
}
template<class buffer_type>
ccl::communicator::coll_request_t
thread_device_group_a2a_communicator::alltoallv_impl(const buffer_type& send_buf,
                                                     const size_t* send_counts,
                                                     buffer_type& recv_buf,
                                                     const size_t* recv_counts,
                                                     const ccl::coll_attr* attr,
                                                     ccl::stream::impl_t& stream)
{
    throw ccl::ccl_error(std::string(__PRETTY_FUNCTION__) + " - is not implemented");
    return {};
}

/* bcast */
template<class buffer_type>
ccl::communicator::coll_request_t
thread_device_group_a2a_communicator::bcast_impl(buffer_type* buf,
                                                 size_t count,
                                                 size_t root,
                                                 const ccl::coll_attr* attr,
                                                 ccl::stream::impl_t& stream)
{
    throw ccl::ccl_error(std::string(__PRETTY_FUNCTION__) + " - is not implemented");
    return {};
}

template<class buffer_type>
ccl::communicator::coll_request_t
thread_device_group_a2a_communicator::bcast_impl(buffer_type& buf,
                                                 size_t count,
                                                 size_t root,
                                                 const ccl::coll_attr* attr,
                                                 ccl::stream::impl_t& stream)
{
    throw ccl::ccl_error(std::string(__PRETTY_FUNCTION__) + " - is not implemented");
    return {};
}

/* reduce */
template<class buffer_type>
ccl::communicator::coll_request_t
thread_device_group_a2a_communicator::reduce_impl(const buffer_type* send_buf,
                                                  buffer_type* recv_buf,
                                                  size_t count,
                                                  ccl::reduction reduction,
                                                  size_t root,
                                                  const ccl::coll_attr* attr,
                                                  ccl::stream::impl_t& stream)
{
    throw ccl::ccl_error(std::string(__PRETTY_FUNCTION__) + " - is not implemented");
    return {};
}

template<class buffer_type>
ccl::communicator::coll_request_t
thread_device_group_a2a_communicator::reduce_impl(const buffer_type& send_buf,
                                                  buffer_type& recv_buf,
                                                  size_t count,
                                                  ccl::reduction reduction,
                                                  size_t root,
                                                  const ccl::coll_attr* attr,
                                                  ccl::stream::impl_t& stream)
{
    throw ccl::ccl_error(std::string(__PRETTY_FUNCTION__) + " - is not implemented");
    return {};
}


/* sparse_allreduce */
template<class index_buffer_type,
         class value_buffer_type>
ccl::communicator::coll_request_t
thread_device_group_a2a_communicator::sparse_allreduce_impl(
                                    const index_buffer_type* send_ind_buf, size_t send_ind_count,
                                    const value_buffer_type* send_val_buf, size_t send_val_count,
                                    index_buffer_type** recv_ind_buf, size_t* recv_ind_count,
                                    value_buffer_type** recv_val_buf, size_t* recv_val_count,
                                    ccl::reduction reduction,
                                    const ccl::coll_attr* attr,
                                    ccl::stream::impl_t& stream)
{
    throw ccl::ccl_error(std::string(__PRETTY_FUNCTION__) + " - is not implemented");
    return {};
}

template<class index_buffer_container_type,
         class value_buffer_container_type>
ccl::communicator::coll_request_t
thread_device_group_a2a_communicator::sparse_allreduce_impl(
                                    const index_buffer_container_type& send_ind_buf, size_t send_ind_count,
                                    const value_buffer_container_type& send_val_buf, size_t send_val_count,
                                    index_buffer_container_type** recv_ind_buf, size_t* recv_ind_count,
                                    value_buffer_container_type** recv_val_buf, size_t* recv_val_count,
                                    ccl::reduction reduction,
                                    const ccl::coll_attr* attr,
                                    ccl::stream::impl_t& stream)
{
    throw ccl::ccl_error(std::string(__PRETTY_FUNCTION__) + " - is not implemented");
    return {};
}
