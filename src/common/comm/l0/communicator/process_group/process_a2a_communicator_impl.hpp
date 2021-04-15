#pragma once
#include "common/comm/l0/communicator/process_group/process_a2a_communicator.hpp"
#include "common/comm/l0/communicator/typed_base_communicator_impl.hpp"

#include "common/comm/l0/devices/devices_declaration.hpp"
#include "common/comm/l0/device_community.hpp"
#include "common/comm/l0/context/process_group_ctx.hpp"
#include "common/comm/l0/scheduler/allied_process_group_scheduler.hpp"
#include "common/event/impls/gpu_event.hpp"

#include "common/comm/l0/communicator/process_group/process_communicator_utils.hpp"
/* allgatherv */
template <class buffer_type>
ccl::event process_a2a_communicator::allgatherv_impl(const buffer_type* send_buf,
                                                     size_t send_count,
                                                     buffer_type* recv_buf,
                                                     const ccl::vector_class<size_t>& recv_counts,
                                                     const ccl::stream::impl_value_t& stream,
                                                     const ccl::allgatherv_attr& attr,
                                                     const ccl::vector_class<ccl::event>& deps) {
    throw ccl::exception(std::string(__PRETTY_FUNCTION__) + " - is not implemented");
    return {};
}
template <class buffer_type>
ccl::event process_a2a_communicator::allgatherv_impl(const buffer_type* send_buf,
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
ccl::event process_a2a_communicator::allgatherv_impl(const buffer_type& send_buf,
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
ccl::event process_a2a_communicator::allgatherv_impl(
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

/* allreduce */
template <class buffer_type>
ccl::event process_a2a_communicator::allreduce_impl(const buffer_type* send_buf,
                                                    buffer_type* recv_buf,
                                                    size_t count,
                                                    ccl::reduction reduction,
                                                    const ccl::stream::impl_value_t& stream,
                                                    const ccl::allreduce_attr& attr,
                                                    const ccl::vector_class<ccl::event>& deps) {
    return allreduce_impl(static_cast<const void*>(send_buf),
                          static_cast<void*>(recv_buf),
                          count,
                          ccl::native_type_info<buffer_type>::dtype,
                          reduction,
                          stream,
                          attr,
                          deps);
}

template <class buffer_type>
ccl::event process_a2a_communicator::allreduce_impl(const buffer_type& send_buf,
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
ccl::event process_a2a_communicator::alltoall_impl(const buffer_type* send_buf,
                                                   buffer_type* recv_buf,
                                                   size_t count,
                                                   const ccl::stream::impl_value_t& stream,
                                                   const ccl::alltoall_attr& attr,
                                                   const ccl::vector_class<ccl::event>& deps) {
    throw ccl::exception(std::string(__PRETTY_FUNCTION__) + " - is not implemented");
    return {};
}
template <class buffer_type>
ccl::event process_a2a_communicator::alltoall_impl(const ccl::vector_class<buffer_type*>& send_buf,
                                                   const ccl::vector_class<buffer_type*>& recv_buf,
                                                   size_t count,
                                                   const ccl::stream::impl_value_t& stream,
                                                   const ccl::alltoall_attr& attr,

                                                   const ccl::vector_class<ccl::event>& deps) {
    throw ccl::exception(std::string(__PRETTY_FUNCTION__) + " - is not implemented");
    return {};
}

template <class buffer_type>
ccl::event process_a2a_communicator::alltoall_impl(const buffer_type& send_buf,
                                                   buffer_type& recv_buf,
                                                   size_t count,
                                                   const ccl::stream::impl_value_t& stream,
                                                   const ccl::alltoall_attr& attr,
                                                   const ccl::vector_class<ccl::event>& deps) {
    throw ccl::exception(std::string(__PRETTY_FUNCTION__) + " - is not implemented");
    return {};
}
template <class buffer_type>
ccl::event process_a2a_communicator::alltoall_impl(
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
ccl::event process_a2a_communicator::alltoallv_impl(const buffer_type* send_buf,
                                                    const ccl::vector_class<size_t>& send_counts,
                                                    buffer_type* recv_buf,
                                                    const ccl::vector_class<size_t>& recv_counts,
                                                    const ccl::stream::impl_value_t& stream,
                                                    const ccl::alltoallv_attr& attr,
                                                    const ccl::vector_class<ccl::event>& deps) {
    throw ccl::exception(std::string(__PRETTY_FUNCTION__) + " - is not implemented");
    return {};
}
template <class buffer_type>
ccl::event process_a2a_communicator::alltoallv_impl(const ccl::vector_class<buffer_type*>& send_buf,
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
ccl::event process_a2a_communicator::alltoallv_impl(const buffer_type& send_buf,
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
ccl::event process_a2a_communicator::alltoallv_impl(
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

/* bcast */
template <class buffer_type>
ccl::event process_a2a_communicator::broadcast_impl(buffer_type* buf,
                                                    size_t count,
                                                    int root,
                                                    const ccl::stream::impl_value_t& stream,
                                                    const ccl::broadcast_attr& attr,
                                                    const ccl::vector_class<ccl::event>& deps) {
    throw ccl::exception(std::string(__PRETTY_FUNCTION__) + " - is not implemented");
    return {};
}

template <class buffer_type>
ccl::event process_a2a_communicator::broadcast_impl(buffer_type& buf,
                                                    size_t count,
                                                    int root,
                                                    const ccl::stream::impl_value_t& stream,
                                                    const ccl::broadcast_attr& attr,
                                                    const ccl::vector_class<ccl::event>& deps) {
    throw ccl::exception(std::string(__PRETTY_FUNCTION__) + " - is not implemented");
    return {};
}

/* reduce */
template <class buffer_type>
ccl::event process_a2a_communicator::reduce_impl(const buffer_type* send_buf,
                                                 buffer_type* recv_buf,
                                                 size_t count,
                                                 ccl::reduction reduction,
                                                 int root,
                                                 const ccl::stream::impl_value_t& stream,
                                                 const ccl::reduce_attr& attr,
                                                 const ccl::vector_class<ccl::event>& deps) {
    throw ccl::exception(std::string(__PRETTY_FUNCTION__) + " - is not implemented");
    return {};
}

template <class buffer_type>
ccl::event process_a2a_communicator::reduce_impl(const buffer_type& send_buf,
                                                 buffer_type& recv_buf,
                                                 size_t count,
                                                 ccl::reduction reduction,
                                                 int root,
                                                 const ccl::stream::impl_value_t& stream,
                                                 const ccl::reduce_attr& attr,
                                                 const ccl::vector_class<ccl::event>& deps) {
    throw ccl::exception(std::string(__PRETTY_FUNCTION__) + " - is not implemented");
    return {};
}
/* reduce_scatter */
template <class buffer_type>
ccl::event process_a2a_communicator::reduce_scatter_impl(
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
ccl::event process_a2a_communicator::reduce_scatter_impl(
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
ccl::event process_a2a_communicator::sparse_allreduce_impl(
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
ccl::event process_a2a_communicator::sparse_allreduce_impl(
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
