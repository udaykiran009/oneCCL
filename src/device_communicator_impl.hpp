#pragma once
#include "ccl_comm_split_attr.hpp"
#include "ccl_comm_split_attr_ids.hpp"
#include "ccl_comm_split_attr_ids_traits.hpp"
#include "ccl_device_communicator.hpp"

//TODO
/*
namespace ccl
{
struct comm_split_attr_impl
{
    constexpr static int color_default()
    {
        return 0;
    }
    ccl_version_t library_version;
};

struct device_attr_impl
{
    constexpr static device_topology_type class_default()
    {
        return device_topology_type::ring;
    }
    constexpr static device_group_split_type group_default()
    {
        return device_group_split_type::process;
    }
    device_topology_type current_preferred_topology_class = class_default();
    device_group_split_type current_preferred_topology_group = group_default();
};
}*/
#include "common/comm/comm_interface.hpp"

namespace ccl
{
/* TODO temporary function for UT compilation: would be part of ccl::environment in final
template <class event_type,
          class ...attr_value_pair_t>
event create_event_from_attr(event_type& native_event_handle,
                             typename unified_device_context_type::ccl_native_t context,
                             attr_value_pair_t&&...avps)
{
    ccl_version_t ret {};
    ret.major = CCL_MAJOR_VERSION;
    ret.minor = CCL_MINOR_VERSION;
    ret.update = CCL_UPDATE_VERSION;
    ret.product_status = CCL_PRODUCT_STATUS;
    ret.build_date = CCL_PRODUCT_BUILD_DATE;
    ret.full = CCL_PRODUCT_FULL;

    event str {event::impl_value_t(new event::impl_t(native_event_handle, context, ret))};
    int expander [] {(str.template set<attr_value_pair_t::idx()>(avps.val()), 0)...};
    str.build_from_params();

    return str;
}
*/


CCL_API device_communicator::device_communicator(impl_value_t&& impl):
    base_t(std::move(impl))
{
}

CCL_API device_communicator::device_communicator(device_communicator&& src) :
    base_t(std::move(src))
{
}

CCL_API device_communicator& device_communicator::operator=(device_communicator&& src)
{
    if (src.get_impl() != this->get_impl())
    {
        src.get_impl().swap(this->get_impl());
        src.get_impl().reset();
    }
    return *this;
}

CCL_API ccl::device_communicator::~device_communicator()
{
}

CCL_API size_t ccl::device_communicator::rank() const
{
    return pimpl->rank();
}

CCL_API size_t ccl::device_communicator::size() const
{
    return pimpl->size();
}

/*
CCL_API ccl::comm_attr_t ccl::device_communicator::get_comm_split_attr() const
{
    return pimpl->get_comm_split_attr();
}

CCL_API ccl::device_group_split_type ccl::device_communicator::get_device_group_split_type() const
{
    return pimpl->get_topology_type();
}

CCL_API ccl::device_topology_type ccl::device_communicator::get_topology_class() const
{
    return pimpl->get_topology_class();
}

*/
CCL_API ccl::device_communicator::ccl_device_t ccl::device_communicator::get_device()
{
    return pimpl->get_device();
}


CCL_API ccl::device_communicator::ccl_context_t ccl::device_communicator::get_context()
{
    return pimpl->get_context();
}


CCL_API bool ccl::device_communicator::is_ready() const
{
    return pimpl->is_ready();
}

/* allgatherv */
//(op_stream) ? op_stream.get_impl() :222 ccl::get_empty_stream().get_impl())111;
ccl::device_communicator::request_t CCL_API
ccl::device_communicator::allgatherv(const void* send_buf,
                         size_t send_count,
                         void* recv_buf,
                         const vector_class<size_t>& recv_counts,
                         datatype dtype,
                         const allgatherv_attr_t& attr,
                         stream op_stream,
                         const vector_class<event>& deps)
{
    static_assert(std::is_base_of<direct_access_policy<ccl_stream>, stream>::value, "stream should provide shared acces to ints pimpl member");
    return pimpl->allgatherv(send_buf, send_count, recv_buf, recv_counts,
                             static_cast<ccl_datatype_t>(dtype),
                             attr,
                             op_stream.get_impl(), deps);
}

CCL_API ccl::device_communicator::request_t
ccl::device_communicator::allgatherv(const void* send_buf,
                         size_t send_count,
                         const vector_class<void*>& recv_bufs,
                         const vector_class<size_t>& recv_counts,
                         datatype dtype,
                         const allgatherv_attr_t& attr,
                         stream op_stream,
                         const vector_class<event>& deps)
{
    return pimpl->allgatherv(send_buf, send_count, recv_bufs, recv_counts,
                            static_cast<ccl_datatype_t>(dtype),
                            attr,
                            op_stream.get_impl(), deps);
}

template<class BufferType,
         typename T>
ccl::device_communicator::request_t CCL_API
ccl::device_communicator::allgatherv(const BufferType* send_buf,
                         size_t send_count,
                         BufferType* recv_buf,
                         const vector_class<size_t>& recv_counts,
                         const allgatherv_attr_t& attr/* = allgatherv_attr_t()*/,
                         stream op_stream,
                         const vector_class<event>& deps)
{
    return pimpl->allgatherv(send_buf, send_count, recv_buf, recv_counts,
                            attr,
                            op_stream.get_impl(), deps);
}
////////////////////////// TODO
template <class BufferType,
          typename T>
ccl::device_communicator::request_t CCL_API
ccl::device_communicator::allgatherv(const BufferType* send_buf,
                         size_t send_count,
                         vector_class<BufferType*>& recv_bufs,
                         const vector_class<size_t>& recv_counts,
                         const allgatherv_attr_t& attr/* = allgatherv_attr_t()*/,
                         stream op_stream,
                         const vector_class<event>& deps)
{
    return pimpl->allgatherv(send_buf, send_count, recv_bufs, recv_counts,
                            attr,
                            op_stream.get_impl(), deps);
}

template <class BufferObjectType,
          typename T>
ccl::device_communicator::request_t CCL_API
ccl::device_communicator::allgatherv(const BufferObjectType& send_buf,
                         size_t send_count,
                         BufferObjectType& recv_buf,
                         const vector_class<size_t>& recv_counts,
                         const allgatherv_attr_t& attr/* = allgatherv_attr_t()*/,
                         stream op_stream,
                         const vector_class<event>& deps)
{
     return pimpl->allgatherv(send_buf, send_count, recv_buf, recv_counts,
                            attr,
                            op_stream.get_impl(), deps);
}

template <class BufferObjectType,
          typename T>
ccl::device_communicator::request_t CCL_API
ccl::device_communicator::allgatherv(const BufferObjectType& send_buf,
                         size_t send_count,
                         vector_class<BufferObjectType&>& recv_bufs,
                         const vector_class<size_t>& recv_counts,
                         const allgatherv_attr_t& attr/* = allgatherv_attr_t()*/,
                         stream op_stream,
                         const vector_class<event>& deps)
{
        return pimpl->allgatherv(send_buf, send_count, recv_bufs, recv_counts,
                            attr,
                            op_stream.get_impl(), deps);
}

/* allreduce */
ccl::device_communicator::request_t CCL_API
ccl::device_communicator::allreduce(const void* send_buf,
                        void* recv_buf,
                        size_t count,
                        datatype dtype,
                        reduction reduction,
                        const allreduce_attr_t& attr,/* = allreduce_attr_t()*/
                        stream op_stream,
                        const vector_class<event>& deps)
{
    return pimpl->allreduce(send_buf, recv_buf, count,
                            static_cast<ccl_datatype_t>(dtype),
                            reduction, attr,
                            op_stream.get_impl(), deps);
}

template<class BufferType,
         typename T>
ccl::device_communicator::request_t CCL_API
ccl::device_communicator::allreduce(const BufferType* send_buf,
                        BufferType* recv_buf,
                        size_t count,
                        reduction reduction,
                        const allreduce_attr_t& attr,/* = allreduce_attr_t()*/
                        stream op_stream,
                        const vector_class<event>& deps)
{
    return pimpl->allreduce(send_buf, recv_buf, count, reduction, attr,
                            op_stream.get_impl(), deps);
}

template<class BufferObjectType,
         typename T>
ccl::device_communicator::request_t CCL_API
ccl::device_communicator::allreduce(const BufferObjectType& send_buf,
                        BufferObjectType& recv_buf,
                        size_t count,
                        reduction reduction,
                        const allreduce_attr_t& attr/* = allreduce_attr_t()*/,
                        stream op_stream,
                        const vector_class<event>& deps)
{
    return pimpl->allreduce(send_buf, recv_buf, count, reduction, attr,
                            op_stream.get_impl(), deps);
}

/* alltoall */
ccl::device_communicator::request_t CCL_API
ccl::device_communicator::alltoall(const void* send_buf,
                       void* recv_buf,
                       size_t count,
                       datatype dtype,
                       const alltoall_attr_t& attr/* = alltoall_attr_t()*/,
                       stream op_stream,
                       const vector_class<event>& deps)
{
    return pimpl->alltoall(send_buf, recv_buf, count,
                           static_cast<ccl_datatype_t>(dtype),
                           attr,
                           op_stream.get_impl(), deps);
}

ccl::device_communicator::request_t CCL_API
ccl::device_communicator::alltoall(const vector_class<void*>& send_buf,
                       const vector_class<void*>& recv_buf,
                       size_t count,
                       datatype dtype,
                       const alltoall_attr_t& attr/* = alltoall_attr_t()*/,
                       stream op_stream,
                       const vector_class<event>& deps)
{
    return pimpl->alltoall(send_buf, recv_buf, count,
                           static_cast<ccl_datatype_t>(dtype),
                           attr,
                           op_stream.get_impl(), deps);
}

template<class BufferType, typename T>
ccl::device_communicator::request_t CCL_API
ccl::device_communicator::alltoall(const BufferType* send_buf,
                       BufferType* recv_buf,
                       size_t count,
                       const alltoall_attr_t& attr/* = alltoall_attr_t()*/,
                       stream op_stream,
                       const vector_class<event>& deps)
{
    return pimpl->alltoall(send_buf, recv_buf, count,
                           attr,
                           op_stream.get_impl(), deps);
}

template<class BufferType, typename T>
ccl::device_communicator::request_t CCL_API
ccl::device_communicator::alltoall(const vector_class<BufferType*>& send_buf,
                       const vector_class<BufferType*>& recv_buf,
                       size_t count,
                       const alltoall_attr_t& attr/* = alltoall_attr_t()*/,
                       stream op_stream,
                       const vector_class<event>& deps)
{
    return pimpl->alltoall(send_buf, recv_buf, count,
                           attr,
                           op_stream.get_impl(), deps);
}

template<class BufferObjectType, typename T>
ccl::device_communicator::request_t CCL_API
ccl::device_communicator::alltoall(const BufferObjectType& send_buf,
                       BufferObjectType& recv_buf,
                       size_t count,
                       const alltoall_attr_t& attr/* = alltoall_attr_t()*/,
                       stream op_stream,
                       const vector_class<event>& deps)
{
    return pimpl->alltoall(send_buf, recv_buf, count, attr,
                           op_stream.get_impl());
}

template<class BufferObjectType, typename T>
ccl::device_communicator::request_t CCL_API
ccl::device_communicator::alltoall(const vector_class<BufferObjectType&>& send_buf,
                       const vector_class<BufferObjectType&>& recv_buf,
                       size_t count,
                       const alltoall_attr_t& attr/* = alltoall_attr_t()*/,
                       stream op_stream,
                       const vector_class<event>& deps)
{
    return pimpl->alltoall(send_buf, recv_buf, count, attr,
                           op_stream.get_impl(), deps);
}


/* alltoallv */
ccl::device_communicator::request_t CCL_API
ccl::device_communicator::alltoallv(const void* send_buf,
                        const vector_class<size_t>& send_counts,
                        void* recv_buf,
                        const vector_class<size_t>& recv_counts,
                        datatype dtype,
                        const alltoallv_attr_t& attr/* = alltoallv_attr_t()*/,
                        stream op_stream,
                        const vector_class<event>& deps)
{
    return pimpl->alltoallv(send_buf, send_counts, recv_buf, recv_counts,
                            static_cast<ccl_datatype_t>(dtype),
                            attr,
                            op_stream.get_impl(), deps);
}


ccl::device_communicator::request_t CCL_API
ccl::device_communicator::alltoallv(const vector_class<void*>& send_bufs,
                        const vector_class<size_t>& send_counts,
                        const vector_class<void*>& recv_bufs,
                        const vector_class<size_t>& recv_counts,
                        datatype dtype,
                        const alltoallv_attr_t& attr/* = alltoallv_attr_t()*/,
                        stream op_stream,
                        const vector_class<event>& deps)
{
    return pimpl->alltoallv(send_bufs, send_counts, recv_bufs, recv_counts,
                            static_cast<ccl_datatype_t>(dtype),
                            attr,
                            op_stream.get_impl(), deps);
}

template<class BufferType, typename T>
ccl::device_communicator::request_t CCL_API
ccl::device_communicator::alltoallv(const BufferType* send_buf,
                        const vector_class<size_t>& send_counts,
                        BufferType* recv_buf,
                        const vector_class<size_t>& recv_counts,
                        const alltoallv_attr_t& attr/* = alltoallv_attr_t()*/,
                        stream op_stream,
                        const vector_class<event>& deps)
{
    return pimpl->alltoallv(send_buf, send_counts, recv_buf, recv_counts,
                            attr,
                            op_stream.get_impl(), deps);
}

template<class BufferType, typename T>
ccl::device_communicator::request_t CCL_API
ccl::device_communicator::alltoallv(const vector_class<BufferType*>& send_bufs,
                        const vector_class<size_t>& send_counts,
                        const vector_class<BufferType*>& recv_bufs,
                        const vector_class<size_t>& recv_counts,
                        const alltoallv_attr_t& attr/* = alltoallv_attr_t()*/,
                        stream op_stream,
                        const vector_class<event>& deps)
{
    return pimpl->alltoallv(send_bufs, send_counts, recv_bufs, recv_counts,
                            attr,
                            op_stream.get_impl(), deps);
}

template<class BufferObjectType, typename T>
ccl::device_communicator::request_t CCL_API
ccl::device_communicator::alltoallv(const BufferObjectType& send_buf,
                        const vector_class<size_t>& send_counts,
                        BufferObjectType& recv_buf,
                        const vector_class<size_t>& recv_counts,
                        const alltoallv_attr_t& attr/* = alltoallv_attr_t()*/,
                        stream op_stream,
                        const vector_class<event>& deps)
{
    return pimpl->alltoallv(send_buf, send_counts, recv_buf, recv_counts,
                            attr,
                            op_stream.get_impl(), deps);
}


template<class BufferObjectType, typename T>
ccl::device_communicator::request_t CCL_API
ccl::device_communicator::alltoallv(const vector_class<BufferObjectType&>& send_bufs,
                        const vector_class<size_t>& send_counts,
                        const vector_class<BufferObjectType&>& recv_bufs,
                        const vector_class<size_t>& recv_counts,
                        const alltoallv_attr_t& attr/* = alltoallv_attr_t()*/,
                        stream op_stream,
                        const vector_class<event>& deps)
{
    return pimpl->alltoallv(send_bufs, send_counts, recv_bufs, recv_counts,
                            attr,
                            op_stream.get_impl(), deps);
}

/* barrier */
ccl::device_communicator::request_t CCL_API
ccl::device_communicator::barrier(const barrier_attr_t& attr/* = barrier_attr_t()*/,
                                  stream op_stream,
                                  const vector_class<event>& deps)
{
    return pimpl->barrier(attr, op_stream.get_impl(), deps);
}

/* bcast */
ccl::device_communicator::request_t CCL_API
ccl::device_communicator::bcast(void* buf,
                    size_t count,
                    datatype dtype,
                    size_t root,
                    const bcast_attr_t& attr/* = bcast_attr_t()*/,
                    stream op_stream,
                    const vector_class<event>& deps)
{
    return pimpl->bcast(buf, count,
                        static_cast<ccl_datatype_t>(dtype),
                        root, attr,
                        op_stream.get_impl(), deps);
}

template<class BufferType,
         typename T>
ccl::device_communicator::request_t CCL_API
ccl::device_communicator::bcast(BufferType* buf,
                    size_t count,
                    size_t root,
                    const bcast_attr_t& attr/* = bcast_attr_t()*/,
                    stream op_stream,
                    const vector_class<event>& deps)

{
    return pimpl->bcast(buf, count, root, attr,
                        op_stream.get_impl(), deps);
}

template<class BufferObjectType,
         typename T>
ccl::device_communicator::request_t CCL_API
ccl::device_communicator::bcast(BufferObjectType& buf,
                    size_t count,
                    size_t root,
                    const bcast_attr_t& attr/* = bcast_attr_t()*/,
                    stream op_stream,
                    const vector_class<event>& deps)
{
    return pimpl->bcast(buf, count, root, attr,
                        op_stream.get_impl(), deps);
}


/* reduce */
ccl::device_communicator::request_t CCL_API
ccl::device_communicator::reduce(const void* send_buf,
                     void* recv_buf,
                     size_t count,
                     datatype dtype,
                     reduction reduction,
                     size_t root,
                     const reduce_attr_t& attr/* = reduce_attr_t()*/,
                     stream op_stream,
                     const vector_class<event>& deps)
{
    return pimpl->reduce(send_buf, recv_buf, count,
                         static_cast<ccl_datatype_t>(dtype),
                         reduction, root, attr,
                         op_stream.get_impl(), deps);
}

template<class BufferType,
         typename T>
ccl::device_communicator::request_t CCL_API
ccl::device_communicator::reduce(const BufferType* send_buf,
                     BufferType* recv_buf,
                     size_t count,
                     reduction reduction,
                     size_t root,
                     const reduce_attr_t& attr/* = reduce_attr_t()*/,
                     stream op_stream,
                     const vector_class<event>& deps)
{
    return pimpl->reduce(send_buf, recv_buf, count,
                         reduction, root, attr,
                         op_stream.get_impl(), deps);
}

template<class BufferObjectType,
         typename T>
ccl::device_communicator::request_t CCL_API
ccl::device_communicator::reduce(const BufferObjectType& send_buf,
                     BufferObjectType& recv_buf,
                     size_t count,
                     reduction reduction,
                     size_t root,
                     const reduce_attr_t& attr/* = reduce_attr_t()*/,
                     stream op_stream,
                     const vector_class<event>& deps)
{
    return pimpl->reduce(send_buf, recv_buf, count,
                         reduction, root, attr,
                         op_stream.get_impl(), deps);
}



/* reduce_scatter */
ccl::device_communicator::request_t CCL_API
ccl::device_communicator::reduce_scatter(const void* send_buf,
                             void* recv_buf,
                             size_t recv_count,
                             datatype dtype,
                             reduction reduction,
                             const reduce_scatter_attr_t& attr/* = reduce_scatter_attr_t()*/,
                             stream op_stream,
                             const vector_class<event>& deps)
{
    return pimpl->reduce_scatter(send_buf, recv_buf, recv_count,
                         static_cast<ccl_datatype_t>(dtype),
                         reduction, attr,
                         op_stream.get_impl(), deps);
}

template<class BufferType,
         typename T>
ccl::device_communicator::request_t CCL_API
ccl::device_communicator::reduce_scatter(const BufferType* send_buf,
                             BufferType* recv_buf,
                             size_t recv_count,
                             reduction reduction,
                             const reduce_scatter_attr_t& attr/* = reduce_scatter_attr_t()*/,
                             stream op_stream,
                             const vector_class<event>& deps)
{
    return pimpl->reduce_scatter(send_buf, recv_buf, recv_count,
                         reduction, attr,
                         op_stream.get_impl(), deps);
}

template<class BufferObjectType,
         typename T>
ccl::device_communicator::request_t CCL_API
ccl::device_communicator::reduce_scatter(const BufferObjectType& send_buf,
                   BufferObjectType& recv_buf,
                   size_t recv_count,
                   reduction reduction,
                   const reduce_scatter_attr_t& attr/* = reduce_scatter_attr_t()*/,
                   stream op_stream,
                   const vector_class<event>& deps)
{
    return pimpl->reduce_scatter(send_buf, recv_buf, recv_count,
                         reduction, attr,
                         op_stream.get_impl(), deps);
}

/* sparse_allreduce */
ccl::device_communicator::request_t CCL_API
ccl::device_communicator::sparse_allreduce(const void* send_ind_buf,
                               size_t send_ind_count,
                               const void* send_val_buf,
                               size_t send_val_count,
                               void* recv_ind_buf,
                               size_t recv_ind_count,
                               void* recv_val_buf,
                               size_t recv_val_count,
                               datatype index_dtype,
                               datatype value_dtype,
                               reduction reduction,
                               const sparse_allreduce_attr_t& attr/* = sparse_allreduce_attr_t()*/,
                               stream op_stream,
                               const vector_class<event>& deps)
{
    return pimpl->sparse_allreduce(send_ind_buf, send_ind_count,
                                   send_val_buf, send_val_count,
                                   recv_ind_buf, recv_ind_count,
                                   recv_val_buf, recv_val_count,
                                   static_cast<ccl_datatype_t>(index_dtype),
                                   static_cast<ccl_datatype_t>(value_dtype),
                                   reduction,
                                   attr,
                                   op_stream.get_impl(), deps);
}

template<class index_BufferType,
         class value_BufferType,
         typename T>
ccl::device_communicator::request_t CCL_API
ccl::device_communicator::sparse_allreduce(const index_BufferType* send_ind_buf,
                               size_t send_ind_count,
                               const value_BufferType* send_val_buf,
                               size_t send_val_count,
                               index_BufferType* recv_ind_buf,
                               size_t recv_ind_count,
                               value_BufferType* recv_val_buf,
                               size_t recv_val_count,
                               reduction reduction,
                               const sparse_allreduce_attr_t& attr/* = sparse_allreduce_attr_t()*/,
                               stream op_stream,
                               const vector_class<event>& deps)
{
    return pimpl->sparse_allreduce(send_ind_buf, send_ind_count,
                                   send_val_buf, send_val_count,
                                   recv_ind_buf, recv_ind_count,
                                   recv_val_buf, recv_val_count,
                                   reduction,
                                   attr,
                                   op_stream.get_impl(), deps);
}

template<class index_BufferObjectType,
         class value_BufferObjectType,
         typename T>
ccl::device_communicator::request_t CCL_API
ccl::device_communicator::sparse_allreduce(const index_BufferObjectType& send_ind_buf,
                               size_t send_ind_count,
                               const value_BufferObjectType& send_val_buf,
                               size_t send_val_count,
                               index_BufferObjectType& recv_ind_buf,
                               size_t recv_ind_count,
                               value_BufferObjectType& recv_val_buf,
                               size_t recv_val_count,
                               reduction reduction,
                               const sparse_allreduce_attr_t& attr/* = sparse_allreduce_attr_t()*/,
                               stream op_stream,
                               const vector_class<event>& deps)
{
    return pimpl->sparse_allreduce(send_ind_buf, send_ind_count,
                                   send_val_buf, send_val_count,
                                   recv_ind_buf, recv_ind_count,
                                   recv_val_buf, recv_val_count,
                                   reduction,
                                   attr,
                                   op_stream.get_impl(), deps);
}


}
