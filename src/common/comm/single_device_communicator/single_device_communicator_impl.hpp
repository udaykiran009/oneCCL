#pragma once
#include "common/comm/single_device_communicator/single_device_communicator.hpp"
#include "common/comm/l0/communicator/typed_base_communicator_impl.hpp"

#include "common/comm/l0/devices/devices_declaration.hpp"
#include "common/comm/l0/device_community.hpp"

/* allgatherv */
template<class buffer_type>
ccl::coll_request_t
single_device_communicator::allgatherv_impl(const buffer_type* send_buf,
                                                size_t send_count,
                                                buffer_type* recv_buf,
                                                const ccl::vector_class<size_t>& recv_counts,
                                                const ccl::allgatherv_attr_t& attr,
                                                ccl::stream::impl_value_t& stream, const ccl::vector_class<ccl::event>& deps)
{
    throw ccl::ccl_error(std::string(__PRETTY_FUNCTION__) + " - is not implemented");
    return {};
}
template<class buffer_type>
ccl::coll_request_t
single_device_communicator::allgatherv_impl(const buffer_type* send_buf,
                                             size_t send_count,
                                             ccl::vector_class<buffer_type*>& recv_buf,
                                             const ccl::vector_class<size_t>& recv_counts,
                                             const ccl::allgatherv_attr_t& attr,
                                             ccl::stream::impl_value_t& stream,
                                             const ccl::vector_class<ccl::event>& deps)
{
    throw ccl::ccl_error(std::string(__PRETTY_FUNCTION__) + " - is not implemented");
    return {};
}

template<class buffer_type>
ccl::coll_request_t
single_device_communicator::allgatherv_impl(const buffer_type& send_buf,
                                                size_t send_count,
                                                buffer_type& recv_buf,
                                                const ccl::vector_class<size_t>& recv_counts,
                                                const ccl::allgatherv_attr_t& attr,
                                                ccl::stream::impl_value_t& stream, const ccl::vector_class<ccl::event>& deps)
{
    throw ccl::ccl_error(std::string(__PRETTY_FUNCTION__) + " - is not implemented");
    return {};
}
template<class buffer_type>
ccl::request_t single_device_communicator::allgatherv_impl(const buffer_type& send_buf,
                                             size_t send_count,
                                             ccl::vector_class<ccl::reference_wrapper_class<buffer_type>>& recv_buf,
                                             const ccl::vector_class<size_t>& recv_counts,
                                             const ccl::allgatherv_attr_t& attr,
                                             ccl::stream::impl_value_t& stream,
                                             const ccl::vector_class<ccl::event>& deps)
{
    throw ccl::ccl_error(std::string(__PRETTY_FUNCTION__) + " - is not implemented");
    return {};
}

/* allreduce */
template<class buffer_type>
ccl::coll_request_t
single_device_communicator::allreduce_impl(const buffer_type* send_buf,
                                               buffer_type* recv_buf,
                                               size_t count,
                                               ccl::reduction reduction,
                                               const ccl::allreduce_attr_t& attr,
                                               ccl::stream::impl_value_t& stream, const ccl::vector_class<ccl::event>& deps)
{
    using namespace native;

    static constexpr ccl::device_group_split_type group_id = base_t::topology_type();
    static constexpr ccl::device_topology_type class_id = base_t::topology_class();

    if(!is_ready())
    {
        throw ccl::ccl_error(std::string("Single device communicator for group_id: " + ::to_string(group_id) +
                                         ", class_id: " + ::to_string(class_id) +
                                         " is not ready yet. Not all сommunicators are created in group. Please create them before usage"));
    }

    size_t comm_rank = rank();
    LOG_DEBUG("communicator for device idx: ", get_device_path(),
              ", rank idx: ", comm_rank);

    throw ccl::ccl_error(std::string(__PRETTY_FUNCTION__) + " - is not implemented");
    return {};
}

template<class buffer_type>
ccl::coll_request_t
single_device_communicator::allreduce_impl(const buffer_type& send_buf,
                                               buffer_type& recv_buf,
                                               size_t count,
                                               ccl::reduction reduction,
                                               const ccl::allreduce_attr_t& attr,
                                               ccl::stream::impl_value_t& stream, const ccl::vector_class<ccl::event>& deps)
{
    throw ccl::ccl_error(std::string(__PRETTY_FUNCTION__) + " - is not implemented");
    return {};
}


/* alltoall */
template<class buffer_type>
ccl::coll_request_t
single_device_communicator::alltoall_impl(const buffer_type* send_buf,
                                              buffer_type* recv_buf,
                                              size_t count,
                                              const ccl::alltoall_attr_t& attr,
                                              ccl::stream::impl_value_t& stream, const ccl::vector_class<ccl::event>& deps)
{
    throw ccl::ccl_error(std::string(__PRETTY_FUNCTION__) + " - is not implemented");
    return {};
}
template<class buffer_type>
ccl::request_t
single_device_communicator::alltoall_impl(const ccl::vector_class<buffer_type*>& send_buf,
                                                   const ccl::vector_class<buffer_type*>& recv_buf,
                                                   size_t count,
                                                   const ccl::alltoall_attr_t& attr,
                                                   ccl::stream::impl_value_t& stream,
                                                   const ccl::vector_class<ccl::event>& deps)
{
    throw ccl::ccl_error(std::string(__PRETTY_FUNCTION__) + " - is not implemented");
    return {};
}

template<class buffer_type>
ccl::coll_request_t
single_device_communicator::alltoall_impl(const buffer_type& send_buf,
                                              buffer_type& recv_buf,
                                              size_t count,
                                              const ccl::alltoall_attr_t& attr,
                                              ccl::stream::impl_value_t& stream, const ccl::vector_class<ccl::event>& deps)
{
    throw ccl::ccl_error(std::string(__PRETTY_FUNCTION__) + " - is not implemented");
    return {};
}
template<class buffer_type>
ccl::request_t
single_device_communicator::alltoall_impl(const ccl::vector_class<ccl::reference_wrapper_class<buffer_type>>& send_buf,
                              const ccl::vector_class<ccl::reference_wrapper_class<buffer_type>>& recv_buf,
                                                size_t count,
                                                const ccl::alltoall_attr_t& attr,
                                                ccl::stream::impl_value_t& stream,
                                                const ccl::vector_class<ccl::event>& dep)
{
    throw ccl::ccl_error(std::string(__PRETTY_FUNCTION__) + " - is not implemented");
    return {};
}



/* alltoallv */
template<class buffer_type>
ccl::coll_request_t
single_device_communicator::alltoallv_impl(const buffer_type* send_buf,
                                               const ccl::vector_class<size_t>& send_counts,
                                               buffer_type* recv_buf,
                                               const ccl::vector_class<size_t>& recv_counts,
                                               const ccl::alltoallv_attr_t& attr,
                                               ccl::stream::impl_value_t& stream, const ccl::vector_class<ccl::event>& deps)
{
    throw ccl::ccl_error(std::string(__PRETTY_FUNCTION__) + " - is not implemented");
    return {};
}
template<class buffer_type>
ccl::coll_request_t
single_device_communicator::alltoallv_impl(const ccl::vector_class<buffer_type*>& send_buf,
                                                 const ccl::vector_class<size_t>& send_counts,
                                                 const ccl::vector_class<buffer_type*>& recv_buf,
                                                 const ccl::vector_class<size_t>& recv_counts,
                                                 const ccl::alltoallv_attr_t& attr,
                                                 ccl::stream::impl_value_t& stream,
                                                 const ccl::vector_class<ccl::event>& dep)
{
    throw ccl::ccl_error(std::string(__PRETTY_FUNCTION__) + " - is not implemented");
    return {};
}

template<class buffer_type>
ccl::coll_request_t
single_device_communicator::alltoallv_impl(const buffer_type& send_buf,
                                               const ccl::vector_class<size_t>& send_counts,
                                               buffer_type& recv_buf,
                                               const ccl::vector_class<size_t>& recv_counts,
                                               const ccl::alltoallv_attr_t& attr,
                                               ccl::stream::impl_value_t& stream, const ccl::vector_class<ccl::event>& deps)
{
    throw ccl::ccl_error(std::string(__PRETTY_FUNCTION__) + " - is not implemented");
    return {};
}
template<class buffer_type>
ccl::coll_request_t
single_device_communicator::alltoallv_impl(const ccl::vector_class<ccl::reference_wrapper_class<buffer_type>>& send_buf,
                             const ccl::vector_class<size_t>& send_counts,
                             const ccl::vector_class<ccl::reference_wrapper_class<buffer_type>>& recv_buf,
                             const ccl::vector_class<size_t>& recv_counts,
                             const ccl::alltoallv_attr_t& attr,
                             ccl::stream::impl_value_t& stream,
                             const ccl::vector_class<ccl::event>& dep)
{
    throw ccl::ccl_error(std::string(__PRETTY_FUNCTION__) + " - is not implemented");
    return {};
}


/* bcast */
template<class buffer_type>
ccl::coll_request_t
single_device_communicator::bcast_impl(buffer_type* buf,
                                           size_t count,
                                           size_t root,
                                           const ccl::bcast_attr_t& attr,
                                           ccl::stream::impl_value_t& stream, const ccl::vector_class<ccl::event>& deps)
{
    throw ccl::ccl_error(std::string(__PRETTY_FUNCTION__) + " - is not implemented");
    return {};
}

template<class buffer_type>
ccl::coll_request_t
single_device_communicator::bcast_impl(buffer_type& buf,
                                           size_t count,
                                           size_t root,
                                           const ccl::bcast_attr_t& attr,
                                           ccl::stream::impl_value_t& stream, const ccl::vector_class<ccl::event>& deps)
{
    throw ccl::ccl_error(std::string(__PRETTY_FUNCTION__) + " - is not implemented");
    return {};
}


/* reduce */
template<class buffer_type>
ccl::coll_request_t
single_device_communicator::reduce_impl(const buffer_type* send_buf,
                                            buffer_type* recv_buf,
                                            size_t count,
                                            ccl::reduction reduction,
                                            size_t root,
                                            const ccl::reduce_attr_t& attr,
                                            ccl::stream::impl_value_t& stream, const ccl::vector_class<ccl::event>& deps)
{
    throw ccl::ccl_error(std::string(__PRETTY_FUNCTION__) + " - is not implemented");
    return {};
}

template<class buffer_type>
ccl::coll_request_t
single_device_communicator::reduce_impl(const buffer_type& send_buf,
                                            buffer_type& recv_buf,
                                            size_t count,
                                            ccl::reduction reduction,
                                            size_t root,
                                            const ccl::reduce_attr_t& attr,
                                            ccl::stream::impl_value_t& stream, const ccl::vector_class<ccl::event>& deps)
{
    throw ccl::ccl_error(std::string(__PRETTY_FUNCTION__) + " - is not implemented");
    return {};
}
/* reduce_scatter */
template<class buffer_type>
ccl::coll_request_t
single_device_communicator::reduce_scatter_impl(const buffer_type* send_buf,
                             buffer_type* recv_buf,
                             size_t recv_count,
                             ccl::reduction reduction,
                             const ccl::reduce_scatter_attr_t& attr/* = reduce_scatter_attr_t()*/,
                             ccl::stream::impl_value_t& op_stream,
                             const ccl::vector_class<ccl::event>& deps)
{
    throw ccl::ccl_error(std::string(__PRETTY_FUNCTION__) + " - is not implemented");
    return {};
}
template<class buffer_type>
ccl::coll_request_t
single_device_communicator::reduce_scatter_impl(const buffer_type& send_buf,
                             buffer_type& recv_buf,
                             size_t recv_count,
                             ccl::reduction reduction,
                             const ccl::reduce_scatter_attr_t& attr/* = reduce_scatter_attr_t()*/,
                             ccl::stream::impl_value_t& op_stream,
                             const ccl::vector_class<ccl::event>& deps)
{
    throw ccl::ccl_error(std::string(__PRETTY_FUNCTION__) + " - is not implemented");
    return {};
}



/* sparse_allreduce */
template<class index_buffer_type,
         class value_buffer_type>
ccl::coll_request_t
single_device_communicator::sparse_allreduce_impl(
                                    const index_buffer_type* send_ind_buf, size_t send_ind_count,
                                    const value_buffer_type* send_val_buf, size_t send_val_count,
                                    index_buffer_type* recv_ind_buf, size_t recv_ind_count,
                                    value_buffer_type* recv_val_buf, size_t recv_val_count,
                                    ccl::reduction reduction,
                                    const ccl::sparse_allreduce_attr_t& attr,
                                    ccl::stream::impl_value_t& stream, const ccl::vector_class<ccl::event>& deps)
{
    throw ccl::ccl_error(std::string(__PRETTY_FUNCTION__) + " - is not implemented");
    return {};
}

template<class index_buffer_container_type,
         class value_buffer_container_type>
ccl::coll_request_t
single_device_communicator::sparse_allreduce_impl(
                                    const index_buffer_container_type& send_ind_buf, size_t send_ind_count,
                                    const value_buffer_container_type& send_val_buf, size_t send_val_count,
                                    index_buffer_container_type& recv_ind_buf, size_t recv_ind_count,
                                    value_buffer_container_type& recv_val_buf, size_t recv_val_count,
                                    ccl::reduction reduction,
                                    const ccl::sparse_allreduce_attr_t& attr,
                                    ccl::stream::impl_value_t& stream, const ccl::vector_class<ccl::event>& deps)
{
    throw ccl::ccl_error(std::string(__PRETTY_FUNCTION__) + " - is not implemented");
    return {};
}
