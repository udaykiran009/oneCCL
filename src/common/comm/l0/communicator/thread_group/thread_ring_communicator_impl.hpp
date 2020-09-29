#pragma once
#include "common/comm/l0/communicator/thread_group/thread_ring_communicator.hpp"
#include "common/comm/l0/communicator/typed_base_communicator_impl.hpp"

#include "common/comm/l0/devices/devices_declaration.hpp"
#include "common/comm/l0/device_community.hpp"
#include "common/comm/l0/context/thread_group_ctx.hpp"
#include "common/comm/l0/scheduler/thread_group_scheduler.hpp"
#include "common/request/gpu_request.hpp"

/* allgatherv */
template <class buffer_type>
thread_device_group_ring_communicator::coll_request_t
thread_device_group_ring_communicator::allgatherv_impl(const buffer_type& send_buf,
                                                       size_t send_count,
                                                       buffer_type& recv_buf,
                                                       const ccl::vector_class<size_t>& recv_counts,
                                                       const ccl::stream::impl_value_t& stream,
                                                       const ccl::allgatherv_attr& attr,
                                                       const ccl::vector_class<ccl::event>& deps) {
    throw ccl::exception(std::string(__PRETTY_FUNCTION__) + " - is not implemented");
    return {};
}
template <class buffer_type>
thread_device_group_ring_communicator::coll_request_t
thread_device_group_ring_communicator::allgatherv_impl(const buffer_type* send_buf,
                                                       size_t send_count,
                                                       ccl::vector_class<buffer_type*>& recv_buf,
                                                       const ccl::vector_class<size_t>& recv_counts,
                                                       const ccl::stream::impl_value_t& stream,
                                                       const ccl::allgatherv_attr& attr,

                                                       const ccl::vector_class<ccl::event>& deps) {
    throw ccl::exception(std::string(__PRETTY_FUNCTION__) + " - is not implemented");
    return {};
}

template <class buffer_type>
thread_device_group_ring_communicator::coll_request_t
thread_device_group_ring_communicator::allgatherv_impl(
    const buffer_type& send_buf,
    size_t send_count,
    ccl::vector_class<ccl::reference_wrapper_class<buffer_type>>& recv_buf,
    const ccl::vector_class<size_t>& recv_counts,
    const ccl::stream::impl_value_t& stream,
    const ccl::allgatherv_attr& attr,

    const ccl::vector_class<ccl::event>& deps) {
    throw ccl::exception(std::string(__PRETTY_FUNCTION__) + " - is not implemented");
    return {};
}

template <class buffer_type>
thread_device_group_ring_communicator::coll_request_t
thread_device_group_ring_communicator::allgatherv_impl(const buffer_type* send_buf,
                                                       size_t send_count,
                                                       buffer_type* recv_buf,
                                                       const ccl::vector_class<size_t>& recv_counts,
                                                       const ccl::stream::impl_value_t& stream,
                                                       const ccl::allgatherv_attr& attr,
                                                       const ccl::vector_class<ccl::event>& deps) {
    using namespace native;

    static constexpr ccl::group_split_type group_id = base_t::topology_type();
    static constexpr ccl::device_topology_type class_id = base_t::topology_class();

    if (!is_ready()) {
        throw ccl::exception(std::string(
            "Device communicator for group_id: " + ::to_string(group_id) +
            " is not ready yet. Not all сommunicators are created in group. Please create them before usage"));
    }

    size_t comm_rank = rank();
    size_t ring_index = 0;
    LOG_DEBUG("communicator for device idx: ",
              get_device_path(),
              ", rank idx: ",
              comm_rank,
              ", ring_index :",
              ring_index);

    //TODO make const!
    ccl_buffer send_entry_buffer(const_cast<buffer_type**>(&send_buf),
                                 send_count * sizeof(buffer_type),
                                 0,
                                 ccl_buffer_type::INDIRECT);
    ccl_buffer recv_entry_buffer(
        &recv_buf, send_count * sizeof(buffer_type), 0, ccl_buffer_type::INDIRECT);

    using community_t = typename device_community_container<class_id>::element_type;
    community_t community = device_community_impl.get_topology(ring_index);

    const auto& in_process_gpu_storage = community->get_devices<ccl_gpu_comm>();
    const auto& virtual_process_gpu_storage = community->get_devices<ccl_virtual_gpu_comm>();

    auto& ipc_gpu_storage = community->get_devices<ccl_ipc_gpu_comm>();
    (void)ipc_gpu_storage;

    thread_group_scheduler::thread_schedule_ptr schedule;
    //source for collective operation is real gpu or virtual gpu
    auto real_device_it = in_process_gpu_storage.find(comm_rank);
    if (real_device_it != in_process_gpu_storage.end()) {
        LOG_DEBUG("Invoke: ", real_device_it->second->to_string());

        using gpu_allgatherv_entry = l0_allgatherv_typed_entry<buffer_type, ccl_gpu_comm, group_id>;

        schedule = ctx->scheduler_impl
                       ->submit_entry<gpu_allgatherv_entry, ccl_sched_add_back, group_id, class_id>(
                           thread_id,
                           *community,
                           real_device_it->second,
                           send_entry_buffer,
                           send_count,
                           recv_entry_buffer,
                           recv_counts.data(),
                           stream);
    }
    else {
        auto virtual_device_it = virtual_process_gpu_storage.find(comm_rank);
        if (virtual_device_it != virtual_process_gpu_storage.end()) {
            LOG_DEBUG("Invoke: ", virtual_device_it->second->to_string());
            using gpu_allgatherv_entry =
                l0_allgatherv_typed_entry<buffer_type, ccl_virtual_gpu_comm, group_id>;

            schedule =
                ctx->scheduler_impl
                    ->submit_entry<gpu_allgatherv_entry, ccl_sched_add_back, group_id, class_id>(
                        thread_id,
                        *community,
                        virtual_device_it->second,
                        send_entry_buffer,
                        send_count,
                        recv_entry_buffer,
                        recv_counts.data(),
                        stream);
        }
    }

    //if sched is not ready - send NULL
    if (schedule) {
        LOG_DEBUG("Device group finalized");
    }
    return std::unique_ptr<ccl::request_impl>(
        new ccl::gpu_shared_request_impl(std::move(schedule)));
}

/* allreduce */
template <class buffer_type>
thread_device_group_ring_communicator::coll_request_t
thread_device_group_ring_communicator::allreduce_impl(const buffer_type* send_buf,
                                                      buffer_type* recv_buf,
                                                      size_t count,
                                                      ccl::reduction reduction,
                                                      const ccl::stream::impl_value_t& stream,
                                                      const ccl::allreduce_attr& attr,
                                                      const ccl::vector_class<ccl::event>& deps) {
    using namespace native;

    static constexpr ccl::group_split_type group_id = base_t::topology_type();
    static constexpr ccl::device_topology_type class_id = base_t::topology_class();

    if (!is_ready()) {
        throw ccl::exception(std::string(
            "Device communicator for group_id: " + ::to_string(group_id) +
            " is not ready yet. Not all сommunicators are created in group. Please create them before usage"));
    }

    size_t comm_rank = rank();
    size_t ring_index = 0;
    LOG_DEBUG("communicator for device idx: ",
              get_device_path(),
              ", rank idx: ",
              comm_rank,
              ", ring_index :",
              ring_index);

    //TODO make const!
    ccl_buffer send_entry_buffer(const_cast<buffer_type**>(&send_buf),
                                 count * sizeof(buffer_type),
                                 0,
                                 ccl_buffer_type::INDIRECT);
    ccl_buffer recv_entry_buffer(
        &recv_buf, count * sizeof(buffer_type), 0, ccl_buffer_type::INDIRECT);

    using community_t = typename device_community_container<class_id>::element_type;
    community_t community = device_community_impl.get_topology(ring_index);

    const auto& in_process_gpu_storage = community->get_devices<ccl_gpu_comm>();
    const auto& virtual_process_gpu_storage = community->get_devices<ccl_virtual_gpu_comm>();

    auto& ipc_gpu_storage = community->get_devices<ccl_ipc_gpu_comm>();
    (void)ipc_gpu_storage;

    thread_group_scheduler::thread_schedule_ptr schedule;
    //source for collective operation is real gpu or virtual gpu
    auto real_device_it = in_process_gpu_storage.find(comm_rank);
    if (real_device_it != in_process_gpu_storage.end()) {
        LOG_DEBUG("Invoke: ", real_device_it->second->to_string());

        using gpu_allreduce_entry = l0_allreduce_typed_entry<buffer_type, ccl_gpu_comm, group_id>;

        schedule = ctx->scheduler_impl
                       ->submit_entry<gpu_allreduce_entry, ccl_sched_add_back, group_id, class_id>(
                           thread_id,
                           *community,
                           real_device_it->second,
                           send_entry_buffer,
                           recv_entry_buffer,
                           count,
                           reduction,
                           stream);
    }
    else {
        auto virtual_device_it = virtual_process_gpu_storage.find(comm_rank);
        if (virtual_device_it != virtual_process_gpu_storage.end()) {
            LOG_DEBUG("Invoke: ", virtual_device_it->second->to_string());
            using gpu_allreduce_entry =
                l0_allreduce_typed_entry<buffer_type, ccl_virtual_gpu_comm, group_id>;

            schedule =
                ctx->scheduler_impl
                    ->submit_entry<gpu_allreduce_entry, ccl_sched_add_back, group_id, class_id>(
                        thread_id,
                        *community,
                        virtual_device_it->second,
                        send_entry_buffer,
                        recv_entry_buffer,
                        count,
                        reduction,
                        stream);
        }
    }

    //if sched is not ready - send NULL
    if (schedule) {
        LOG_DEBUG("Device group finalized");
    }
    return std::unique_ptr<ccl::request_impl>(
        new ccl::gpu_shared_request_impl(std::move(schedule)));
}

template <class buffer_type>
thread_device_group_ring_communicator::coll_request_t
thread_device_group_ring_communicator::allreduce_impl(const buffer_type& send_buf,
                                                      buffer_type& recv_buf,
                                                      size_t count,
                                                      ccl::reduction reduction,
                                                      const ccl::stream::impl_value_t& stream,
                                                      const ccl::allreduce_attr& attr,
                                                      const ccl::vector_class<ccl::event>& deps) {
    throw ccl::exception(std::string(__PRETTY_FUNCTION__) + " - is not implemented");
    return {};
}

/* alltoall */
template <class buffer_type>
thread_device_group_ring_communicator::coll_request_t
thread_device_group_ring_communicator::alltoall_impl(const buffer_type* send_buf,
                                                     buffer_type* recv_buf,
                                                     size_t count,
                                                     const ccl::stream::impl_value_t& stream,
                                                     const ccl::alltoall_attr& attr,
                                                     const ccl::vector_class<ccl::event>& deps) {
    throw ccl::exception(std::string(__PRETTY_FUNCTION__) + " - is not implemented");
    return {};
}
template <class buffer_type>
thread_device_group_ring_communicator::coll_request_t
thread_device_group_ring_communicator::alltoall_impl(
    const ccl::vector_class<buffer_type*>& send_buf,
    const ccl::vector_class<buffer_type*>& recv_buf,
    size_t count,
    const ccl::stream::impl_value_t& stream,
    const ccl::alltoall_attr& attr,
    const ccl::vector_class<ccl::event>& deps) {
    throw ccl::exception(std::string(__PRETTY_FUNCTION__) + " - is not implemented");
    return {};
}

template <class buffer_type>
thread_device_group_ring_communicator::coll_request_t
thread_device_group_ring_communicator::alltoall_impl(const buffer_type& send_buf,
                                                     buffer_type& recv_buf,
                                                     size_t count,
                                                     const ccl::stream::impl_value_t& stream,
                                                     const ccl::alltoall_attr& attr,
                                                     const ccl::vector_class<ccl::event>& deps) {
    throw ccl::exception(std::string(__PRETTY_FUNCTION__) + " - is not implemented");
    return {};
}
template <class buffer_type>
thread_device_group_ring_communicator::coll_request_t
thread_device_group_ring_communicator::alltoall_impl(
    const ccl::vector_class<ccl::reference_wrapper_class<buffer_type>>& send_buf,
    const ccl::vector_class<ccl::reference_wrapper_class<buffer_type>>& recv_buf,
    size_t count,
    const ccl::stream::impl_value_t& stream,
    const ccl::alltoall_attr& attr,
    const ccl::vector_class<ccl::event>& dep) {
    throw ccl::exception(std::string(__PRETTY_FUNCTION__) + " - is not implemented");
    return {};
}

/* alltoallv */
template <class buffer_type>
thread_device_group_ring_communicator::coll_request_t
thread_device_group_ring_communicator::alltoallv_impl(
    const ccl::vector_class<buffer_type*>& send_buf,
    const ccl::vector_class<size_t>& send_counts,
    const ccl::vector_class<buffer_type*>& recv_buf,
    const ccl::vector_class<size_t>& recv_counts,
    const ccl::stream::impl_value_t& stream,
    const ccl::alltoallv_attr& attr,
    const ccl::vector_class<ccl::event>& dep) {
    throw ccl::exception(std::string(__PRETTY_FUNCTION__) + " - is not implemented");
    return {};
}

template <class buffer_type>
thread_device_group_ring_communicator::coll_request_t
thread_device_group_ring_communicator::alltoallv_impl(const buffer_type& send_buf,
                                                      const ccl::vector_class<size_t>& send_counts,
                                                      buffer_type& recv_buf,
                                                      const ccl::vector_class<size_t>& recv_counts,
                                                      const ccl::stream::impl_value_t& stream,
                                                      const ccl::alltoallv_attr& attr,
                                                      const ccl::vector_class<ccl::event>& deps) {
    throw ccl::exception(std::string(__PRETTY_FUNCTION__) + " - is not implemented");
    return {};
}
template <class buffer_type>
thread_device_group_ring_communicator::coll_request_t
thread_device_group_ring_communicator::alltoallv_impl(
    const ccl::vector_class<ccl::reference_wrapper_class<buffer_type>>& send_buf,
    const ccl::vector_class<size_t>& send_counts,
    const ccl::vector_class<ccl::reference_wrapper_class<buffer_type>>& recv_buf,
    const ccl::vector_class<size_t>& recv_counts,
    const ccl::stream::impl_value_t& stream,
    const ccl::alltoallv_attr& attr,

    const ccl::vector_class<ccl::event>& dep) {
    throw ccl::exception(std::string(__PRETTY_FUNCTION__) + " - is not implemented");
    return {};
}

template <class buffer_type>
thread_device_group_ring_communicator::coll_request_t
thread_device_group_ring_communicator::alltoallv_impl(const buffer_type* send_buf,
                                                      const ccl::vector_class<size_t>& send_counts,
                                                      buffer_type* recv_buf,
                                                      const ccl::vector_class<size_t>& recv_counts,
                                                      const ccl::stream::impl_value_t& stream,
                                                      const ccl::alltoallv_attr& attr,
                                                      const ccl::vector_class<ccl::event>& deps) {
    using namespace native;
    static constexpr ccl::group_split_type group_id = base_t::topology_type();
    static constexpr ccl::device_topology_type class_id = base_t::topology_class();

    if (!is_ready()) {
        throw ccl::exception(std::string(
            "Device communicator for group_id: " + ::to_string(group_id) +
            " is not ready yet. Not all сommunicators are created in group. Please create them before usage"));
    }

    size_t comm_rank = rank();
    size_t ring_index = 0;
    LOG_DEBUG("communicator for device idx: ",
              get_device_path(),
              ", rank idx: ",
              comm_rank,
              ", ring_index :",
              ring_index);
    size_t SIZE = 512;
    //TODO make const!
    ccl_buffer send_entry_buffer(const_cast<buffer_type**>(&send_buf),
                                 SIZE * sizeof(buffer_type),
                                 0,
                                 ccl_buffer_type::INDIRECT);
    ccl_buffer recv_entry_buffer(
        &recv_buf, SIZE * sizeof(buffer_type), 0, ccl_buffer_type::INDIRECT);

    using community_t = typename device_community_container<class_id>::element_type;
    community_t community = device_community_impl.get_topology(ring_index);

    const auto& in_process_gpu_storage = community->get_devices<ccl_gpu_comm>();
    const auto& virtual_process_gpu_storage = community->get_devices<ccl_virtual_gpu_comm>();

    auto& ipc_gpu_storage = community->get_devices<ccl_ipc_gpu_comm>();
    (void)ipc_gpu_storage;

    thread_group_scheduler::thread_schedule_ptr schedule;
    //source for collective operation is real gpu or virtual gpu
    auto real_device_it = in_process_gpu_storage.find(comm_rank);
    if (real_device_it != in_process_gpu_storage.end()) {
        LOG_DEBUG("Invoke: ", real_device_it->second->to_string());

        using gpu_alltoallv_entry = l0_alltoallv_typed_entry<buffer_type, ccl_gpu_comm, group_id>;

        schedule = ctx->scheduler_impl
                       ->submit_entry<gpu_alltoallv_entry, ccl_sched_add_back, group_id, class_id>(
                           thread_id,
                           *community,
                           real_device_it->second,
                           send_entry_buffer,
                           send_counts.data(),
                           recv_entry_buffer,
                           recv_counts.data(),
                           stream);
    }
    else {
        auto virtual_device_it = virtual_process_gpu_storage.find(comm_rank);
        if (virtual_device_it != virtual_process_gpu_storage.end()) {
            LOG_DEBUG("Invoke: ", virtual_device_it->second->to_string());
            using gpu_alltoallv_entry =
                l0_alltoallv_typed_entry<buffer_type, ccl_virtual_gpu_comm, group_id>;

            schedule =
                ctx->scheduler_impl
                    ->submit_entry<gpu_alltoallv_entry, ccl_sched_add_back, group_id, class_id>(
                        thread_id,
                        *community,
                        virtual_device_it->second,
                        send_entry_buffer,
                        send_counts.data(),
                        recv_entry_buffer,
                        recv_counts.data(),
                        stream);
        }
    }

    //if sched is not ready - send NULL
    if (schedule) {
        LOG_DEBUG("Device group finalized");
    }
    return std::unique_ptr<ccl::request_impl>(
        new ccl::gpu_shared_request_impl(std::move(schedule)));
}

/* bcast */
template <class buffer_type>
thread_device_group_ring_communicator::coll_request_t
thread_device_group_ring_communicator::broadcast_impl(buffer_type* buf,
                                                      size_t count,
                                                      size_t root,
                                                      const ccl::stream::impl_value_t& stream,
                                                      const ccl::broadcast_attr& attr,
                                                      const ccl::vector_class<ccl::event>& deps) {
    using namespace native;

    static constexpr ccl::group_split_type group_id = base_t::topology_type();
    static constexpr ccl::device_topology_type class_id = base_t::topology_class();

    if (!is_ready()) {
        throw ccl::exception(std::string(
            "Device communicator for group_id: " + ::to_string(group_id) +
            " is not ready yet. Not all сommunicators are created in group. Please create them before usage"));
    }

    size_t comm_rank = rank();
    size_t ring_index = 0;
    LOG_DEBUG("communicator for device idx: ",
              get_device_path(),
              ", rank idx: ",
              comm_rank,
              ", ring_index :",
              ring_index);

    //TODO make const!
    ccl_buffer entry_buffer(&buf, count * sizeof(buffer_type), 0, ccl_buffer_type::INDIRECT);

    using community_t = typename device_community_container<class_id>::element_type;
    community_t community = device_community_impl.get_topology(ring_index);

    const auto& in_process_gpu_storage = community->get_devices<ccl_gpu_comm>();
    const auto& virtual_process_gpu_storage = community->get_devices<ccl_virtual_gpu_comm>();

    auto& ipc_gpu_storage = community->get_devices<ccl_ipc_gpu_comm>();
    (void)ipc_gpu_storage;

    thread_group_scheduler::thread_schedule_ptr schedule;
    //source for collective operation is real gpu or virtual gpu
    auto real_device_it = in_process_gpu_storage.find(comm_rank);
    if (real_device_it != in_process_gpu_storage.end()) {
        LOG_DEBUG("Invoke: ", real_device_it->second->to_string());

        using gpu_bcast_entry = l0_bcast_typed_entry<buffer_type, ccl_gpu_comm, group_id>;

        schedule = ctx->scheduler_impl
                       ->submit_entry<gpu_bcast_entry, ccl_sched_add_back, group_id, class_id>(
                           thread_id,
                           *community,
                           real_device_it->second,
                           entry_buffer,
                           count,
                           root,
                           stream);
    }
    else {
        auto virtual_device_it = virtual_process_gpu_storage.find(comm_rank);
        if (virtual_device_it != virtual_process_gpu_storage.end()) {
            LOG_DEBUG("Invoke: ", virtual_device_it->second->to_string());
            using gpu_bcast_entry =
                l0_bcast_typed_entry<buffer_type, ccl_virtual_gpu_comm, group_id>;

            schedule = ctx->scheduler_impl
                           ->submit_entry<gpu_bcast_entry, ccl_sched_add_back, group_id, class_id>(
                               thread_id,
                               *community,
                               virtual_device_it->second,
                               entry_buffer,
                               count,
                               root,
                               stream);
        }
    }

    //if sched is not ready - send NULL
    if (schedule) {
        LOG_DEBUG("Device group finalized");
    }
    return std::unique_ptr<ccl::request_impl>(
        new ccl::gpu_shared_request_impl(std::move(schedule)));
}

template <class buffer_type>
thread_device_group_ring_communicator::coll_request_t
thread_device_group_ring_communicator::broadcast_impl(buffer_type& buf,
                                                      size_t count,
                                                      size_t root,
                                                      const ccl::stream::impl_value_t& stream,
                                                      const ccl::broadcast_attr& attr,
                                                      const ccl::vector_class<ccl::event>& deps) {
    throw ccl::exception(std::string(__PRETTY_FUNCTION__) + " - is not implemented");
    return {};
}

/* reduce */
template <class buffer_type>
thread_device_group_ring_communicator::coll_request_t
thread_device_group_ring_communicator::reduce_impl(const buffer_type* send_buf,
                                                   buffer_type* recv_buf,
                                                   size_t count,
                                                   ccl::reduction reduction,
                                                   size_t root,
                                                   const ccl::stream::impl_value_t& stream,
                                                   const ccl::reduce_attr& attr,
                                                   const ccl::vector_class<ccl::event>& deps) {
    using namespace native;

    static constexpr ccl::group_split_type group_id = base_t::topology_type();
    static constexpr ccl::device_topology_type class_id = base_t::topology_class();

    if (!is_ready()) {
        throw ccl::exception(std::string(
            "Device communicator for group_id: " + ::to_string(group_id) +
            " is not ready yet. Not all сommunicators are created in group. Please create them before usage"));
    }

    size_t comm_rank = rank();
    size_t ring_index = 0;
    LOG_DEBUG("communicator for device idx: ",
              get_device_path(),
              ", rank idx: ",
              comm_rank,
              ", ring_index :",
              ring_index);

    //TODO make const!
    ccl_buffer send_entry_buffer(const_cast<buffer_type**>(&send_buf),
                                 count * sizeof(buffer_type),
                                 0,
                                 ccl_buffer_type::INDIRECT);
    ccl_buffer recv_entry_buffer(
        &recv_buf, count * sizeof(buffer_type), 0, ccl_buffer_type::INDIRECT);

    using community_t = typename device_community_container<class_id>::element_type;
    community_t community = device_community_impl.get_topology(ring_index);

    const auto& in_process_gpu_storage = community->get_devices<ccl_gpu_comm>();
    const auto& virtual_process_gpu_storage = community->get_devices<ccl_virtual_gpu_comm>();

    auto& ipc_gpu_storage = community->get_devices<ccl_ipc_gpu_comm>();
    (void)ipc_gpu_storage;

    thread_group_scheduler::thread_schedule_ptr schedule;
    //source for collective operation is real gpu or virtual gpu
    auto real_device_it = in_process_gpu_storage.find(comm_rank);
    if (real_device_it != in_process_gpu_storage.end()) {
        LOG_DEBUG("Invoke: ", real_device_it->second->to_string());

        using gpu_reduce_entry = l0_reduce_typed_entry<buffer_type, ccl_gpu_comm, group_id>;

        schedule = ctx->scheduler_impl
                       ->submit_entry<gpu_reduce_entry, ccl_sched_add_back, group_id, class_id>(
                           thread_id,
                           *community,
                           real_device_it->second,
                           send_entry_buffer,
                           recv_entry_buffer,
                           count,
                           reduction,
                           root,
                           stream);
    }
    else {
        auto virtual_device_it = virtual_process_gpu_storage.find(comm_rank);
        if (virtual_device_it != virtual_process_gpu_storage.end()) {
            LOG_DEBUG("Invoke: ", virtual_device_it->second->to_string());
            using gpu_reduce_entry =
                l0_reduce_typed_entry<buffer_type, ccl_virtual_gpu_comm, group_id>;

            schedule = ctx->scheduler_impl
                           ->submit_entry<gpu_reduce_entry, ccl_sched_add_back, group_id, class_id>(
                               thread_id,
                               *community,
                               virtual_device_it->second,
                               send_entry_buffer,
                               recv_entry_buffer,
                               count,
                               reduction,
                               root,
                               stream);
        }
    }

    //if sched is not ready - send NULL
    if (schedule) {
        LOG_DEBUG("Device group finalized");
    }

    return std::unique_ptr<ccl::request_impl>(
        new ccl::gpu_shared_request_impl(std::move(schedule)));
}

template <class buffer_type>
thread_device_group_ring_communicator::coll_request_t
thread_device_group_ring_communicator::reduce_impl(const buffer_type& send_buf,
                                                   buffer_type& recv_buf,
                                                   size_t count,
                                                   ccl::reduction reduction,
                                                   size_t root,
                                                   const ccl::stream::impl_value_t& stream,
                                                   const ccl::reduce_attr& attr,
                                                   const ccl::vector_class<ccl::event>& deps) {
    throw ccl::exception(std::string(__PRETTY_FUNCTION__) + " - is not implemented");
    return {};
}

/* reduce_scatter */
template <class buffer_type>
thread_device_group_ring_communicator::coll_request_t
thread_device_group_ring_communicator::reduce_scatter_impl(
    const buffer_type* send_buf,
    buffer_type* recv_buf,
    size_t recv_count,
    ccl::reduction reduction,
    const ccl::stream::impl_value_t& stream,
    const ccl::reduce_scatter_attr& attr,
    const ccl::vector_class<ccl::event>& deps) {
    throw ccl::exception(std::string(__PRETTY_FUNCTION__) + " - is not implemented");
    return {};
}
template <class buffer_type>
thread_device_group_ring_communicator::coll_request_t
thread_device_group_ring_communicator::reduce_scatter_impl(
    const buffer_type& send_buf,
    buffer_type& recv_buf,
    size_t recv_count,
    ccl::reduction reduction,
    const ccl::stream::impl_value_t& stream,
    const ccl::reduce_scatter_attr& attr,
    const ccl::vector_class<ccl::event>& deps) {
    throw ccl::exception(std::string(__PRETTY_FUNCTION__) + " - is not implemented");
    return {};
}

/* sparse_allreduce */
template <class index_buffer_type, class value_buffer_type>
thread_device_group_ring_communicator::coll_request_t
thread_device_group_ring_communicator::sparse_allreduce_impl(
    const index_buffer_type* send_ind_buf,
    size_t send_ind_count,
    const value_buffer_type* send_val_buf,
    size_t send_val_count,
    index_buffer_type* recv_ind_buf,
    size_t recv_ind_count,
    value_buffer_type* recv_val_buf,
    size_t recv_val_count,
    ccl::reduction reduction,
    const ccl::stream::impl_value_t& stream,
    const ccl::sparse_allreduce_attr& attr,
    const ccl::vector_class<ccl::event>& deps) {
    throw ccl::exception(std::string(__PRETTY_FUNCTION__) + " - is not implemented");
    return {};
}

template <class index_buffer_container_type, class value_buffer_container_type>
thread_device_group_ring_communicator::coll_request_t
thread_device_group_ring_communicator::sparse_allreduce_impl(
    const index_buffer_container_type& send_ind_buf,
    size_t send_ind_count,
    const value_buffer_container_type& send_val_buf,
    size_t send_val_count,
    index_buffer_container_type& recv_ind_buf,
    size_t recv_ind_count,
    value_buffer_container_type& recv_val_buf,
    size_t recv_val_count,
    ccl::reduction reduction,
    const ccl::stream::impl_value_t& stream,
    const ccl::sparse_allreduce_attr& attr,
    const ccl::vector_class<ccl::event>& deps) {
    throw ccl::exception(std::string(__PRETTY_FUNCTION__) + " - is not implemented");
    return {};
}