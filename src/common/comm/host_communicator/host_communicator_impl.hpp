#pragma once

#include "common/comm/host_communicator/host_communicator.hpp"
#include "common/comm/host_communicator/host_communicator_defines.hpp"

namespace ccl {

/* allgatherv */
template<class BufferType>
ccl::request_t
host_communicator::allgatherv_impl(const BufferType* send_buf,
                                   size_t send_count,
                                   BufferType* recv_buf,
                                   const vector_class<size_t>& recv_counts,
                                   const allgatherv_attr_t& attr)
{
    // TODO There must be a call to a collective function
    return ccl::request_t();
}

template<class BufferType>
ccl::request_t
host_communicator::allgatherv_impl(const BufferType* send_buf,
                                   size_t send_count,
                                   const vector_class<BufferType*>& recv_bufs,
                                   const vector_class<size_t>& recv_counts,
                                   const allgatherv_attr_t& attr)
{
    // TODO There must be a call to a collective function
    return ccl::request_t();
}

/* allreduce */
template <class BufferType,
          typename T>
ccl::request_t
host_communicator::allreduce_impl(const BufferType* send_buf,
                                  BufferType* recv_buf,
                                  size_t count,
                                  ccl_reduction_t reduction,
                                  const allreduce_attr_t& attr)
{
    // TODO There must be a call to a collective function
    return ccl::request_t();
}

/* alltoall */
template <class BufferType,
           typename T>
ccl::request_t
host_communicator::alltoall_impl(const BufferType* send_buf,
                                 BufferType* recv_buf,
                                 size_t count,
                                 const alltoall_attr_t& attr)
{
    // TODO There must be a call to a collective function
    return ccl::request_t();
}

template <class BufferType,
          typename T>
ccl::request_t
host_communicator::alltoall_impl(const vector_class<BufferType*>& send_buf,
                                 const vector_class<BufferType*>& recv_buf,
                                 size_t count,
                                 const alltoall_attr_t& attr)
{
    // TODO There must be a call to a collective function
    return ccl::request_t();
}

/* alltoallv */
template <class BufferType,
          typename T>
ccl::request_t
host_communicator::alltoallv_impl(const BufferType* send_buf,
                                 const vector_class<size_t>& send_counts,
                                 BufferType* recv_buf,
                                 const vector_class<size_t>& recv_counts,
                                 const alltoallv_attr_t& attr)
{
    // TODO There must be a call to a collective function
    return ccl::request_t();
}

template <class BufferType,
          typename T>
ccl::request_t
host_communicator::alltoallv_impl(const vector_class<BufferType*>& send_bufs,
                                  const vector_class<size_t>& send_counts,
                                  const vector_class<BufferType*>& recv_bufs,
                                  const vector_class<size_t>& recv_counts,
                                  const alltoallv_attr_t& attr)
{
    // TODO There must be a call to a collective function
    return ccl::request_t();
}

/* bcast */
template <class BufferType,
          typename T>
ccl::request_t
host_communicator::bcast_impl(BufferType* buf,
                              size_t count,
                              size_t root,
                              const bcast_attr_t& attr)
{
    // TODO There must be a call to a collective function
    return ccl::request_t();
}

/* reduce */
template <class BufferType,
          typename T>
ccl::request_t
host_communicator::reduce_impl(const BufferType* send_buf,
                               BufferType* recv_buf,
                               size_t count,
                               ccl_reduction_t reduction,
                               size_t root,
                               const reduce_attr_t& attr)
{
    // TODO There must be a call to a collective function
    return ccl::request_t();
}

/* reduce_scatter */
template <class BufferType,
          typename T>
ccl::request_t
host_communicator::reduce_scatter_impl(const BufferType* send_buf,
                                       BufferType* recv_buf,
                                       size_t recv_count,
                                       ccl_reduction_t reduction,
                                       const reduce_scatter_attr_t& attr)
{
    // TODO There must be a call to a collective function
    return ccl::request_t();
}

/* sparse_allreduce */
template <
    class index_BufferType,
    class value_BufferType,
    typename T>
ccl::request_t
host_communicator::sparse_allreduce_impl(const index_BufferType* send_ind_buf,
                                         size_t send_ind_count,
                                         const value_BufferType* send_val_buf,
                                         size_t send_val_count,
                                         index_BufferType* recv_ind_buf,
                                         size_t recv_ind_count,
                                         value_BufferType* recv_val_buf,
                                         size_t recv_val_count,
                                         ccl_reduction_t reduction,
                                         const sparse_allreduce_attr_t& attr)
{
    // TODO There must be a call to a collective function
    return ccl::request_t();
}

} // namespace ccl
