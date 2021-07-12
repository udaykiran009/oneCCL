#include "oneapi/ccl/types.hpp"
#include "oneapi/ccl/type_traits.hpp"
//#include "typed_base_communicator_impl.hpp"
#include "common/comm/single_device_communicator/single_device_communicator.hpp"
#include "common/comm/l0/gpu_comm_attr.hpp"

using namespace ccl;

single_device_communicator::single_device_communicator(ccl::unified_device_type&& device,
                                                       ccl::unified_context_type&& ctx,
                                                       size_t thread_idx,
                                                       size_t process_idx,
                                                       const ccl::comm_split_attr& attr)
        : base_t(std::move(device), std::move(ctx), thread_idx, process_idx, /*comm_attr, */ attr) {
}

single_device_communicator::~single_device_communicator() {}

std::shared_ptr<ccl::communicator_interface> single_device_communicator::split(
    const ccl::comm_split_attr& attr) {
    return {};
}

void single_device_communicator::set_ccl_comm(std::shared_ptr<ccl_comm> impl) {
    comm_impl = impl;

    comm_rank = comm_impl->rank();
    comm_size = comm_impl->size();
}

void single_device_communicator::set_context(
    const ccl::unified_context_type::ccl_native_t& in_context) {
    context = in_context;
}
void single_device_communicator::set_context(const ccl::context& in_context) {
    context = in_context.get_native();
}

#ifdef MULTI_GPU_SUPPORT
void single_device_communicator::visit(ccl::gpu_comm_attr& comm_attr) {}
#endif

///////////////

#define TEMPLATE_DECL_ARG class comm_impl, class communicator_traits
#define TEMPLATE_DEF_ARG  comm_impl, communicator_traits

template <TEMPLATE_DECL_ARG>
bool typed_single_device_base_communicator<TEMPLATE_DEF_ARG>::is_ready() const {
    return true;
}

template <TEMPLATE_DECL_ARG>
ccl::group_split_type typed_single_device_base_communicator<TEMPLATE_DEF_ARG>::get_topology_type()
    const {
    return self_t::topology_type();
}

template <TEMPLATE_DECL_ARG>
ccl::device_topology_type
typed_single_device_base_communicator<TEMPLATE_DEF_ARG>::get_topology_class() const {
    return self_t::topology_class();
}

//////////////
template <TEMPLATE_DECL_ARG>
typed_single_device_base_communicator<TEMPLATE_DEF_ARG>::typed_single_device_base_communicator(
    ccl::unified_device_type&& owned_device,
    ccl::unified_context_type&& ctx,
    size_t thread_idx,
    size_t process_idx,
    const ccl::comm_split_attr& attr)
        : base_communicator(std::move(owned_device),
                            std::move(ctx),
                            thread_idx,
                            process_idx /*, comm_attr*/,
                            attr) {}
template <TEMPLATE_DECL_ARG>
std::string typed_single_device_base_communicator<TEMPLATE_DEF_ARG>::to_string() const {
    return {};
}

ccl::event single_device_communicator::barrier(const ccl::stream::impl_value_t& stream,
                                               const ccl::barrier_attr& attr,
                                               const ccl::vector_class<ccl::event>& deps) {
    return {};
}

/* allgatherv */

ccl::event single_device_communicator::allgatherv_base_impl(
    const void* send_buf,
    size_t send_count,
    void* recv_buf,
    const ccl::vector_class<size_t>& recv_counts,
    ccl::datatype dtype,
    const ccl::stream::impl_value_t& stream,
    const ccl_coll_attr& attr,
    const ccl::vector_class<ccl::event>& deps) {
    return {};
}
ccl::event single_device_communicator::allgatherv_impl(const void* send_buf,
                                                       size_t send_count,
                                                       void* recv_buf,
                                                       const ccl::vector_class<size_t>& recv_counts,
                                                       ccl::datatype dtype,

                                                       const ccl::stream::impl_value_t& stream,
                                                       const ccl::allgatherv_attr& attr,
                                                       const ccl::vector_class<ccl::event>& deps) {
    return {};
}
ccl::event single_device_communicator::allgatherv_impl(const void* send_buf,
                                                       size_t send_count,
                                                       const ccl::vector_class<void*>& recv_bufs,
                                                       const ccl::vector_class<size_t>& recv_counts,
                                                       ccl::datatype dtype,

                                                       const ccl::stream::impl_value_t& stream,
                                                       const ccl::allgatherv_attr& attr,
                                                       const ccl::vector_class<ccl::event>& deps) {
    return {};
}

/* allreduce */
ccl::event single_device_communicator::allreduce_impl(const void* send_buf,
                                                      void* recv_buf,
                                                      size_t count,
                                                      ccl::datatype dtype,
                                                      ccl::reduction reduction,

                                                      const ccl::stream::impl_value_t& stream,
                                                      const ccl::allreduce_attr& attr,
                                                      const ccl::vector_class<ccl::event>& deps) {
    return {};
}

/* alltoall */
ccl::event single_device_communicator::alltoall_impl(const void* send_buf,
                                                     void* recv_buf,
                                                     size_t count,
                                                     ccl::datatype dtype,

                                                     const ccl::stream::impl_value_t& stream,
                                                     const ccl::alltoall_attr& attr,
                                                     const ccl::vector_class<ccl::event>& deps) {
    return {};
}
ccl::event single_device_communicator::alltoall_impl(const ccl::vector_class<void*>& send_buf,
                                                     const ccl::vector_class<void*>& recv_buf,
                                                     size_t count,
                                                     ccl::datatype dtype,
                                                     const ccl::stream::impl_value_t& stream,
                                                     const ccl::alltoall_attr& attr,
                                                     const ccl::vector_class<ccl::event>& deps) {
    return {};
}

/* alltoallv */
ccl::event single_device_communicator::alltoallv_impl(const void* send_buf,
                                                      const ccl::vector_class<size_t>& send_counts,
                                                      void* recv_buf,
                                                      const ccl::vector_class<size_t>& recv_counts,
                                                      ccl::datatype dtype,

                                                      const ccl::stream::impl_value_t& stream,
                                                      const ccl::alltoallv_attr& attr,
                                                      const ccl::vector_class<ccl::event>& deps) {
    return {};
}
ccl::event single_device_communicator::alltoallv_impl(const ccl::vector_class<void*>& send_buf,
                                                      const ccl::vector_class<size_t>& send_counts,
                                                      ccl::vector_class<void*> recv_buf,
                                                      const ccl::vector_class<size_t>& recv_counts,
                                                      ccl::datatype dtype,

                                                      const ccl::stream::impl_value_t& stream,
                                                      const ccl::alltoallv_attr& attr,
                                                      const ccl::vector_class<ccl::event>& dep) {
    return {};
}

/* bcast */
ccl::event single_device_communicator::broadcast_impl(void* buf,
                                                      size_t count,
                                                      ccl::datatype dtype,
                                                      int root,

                                                      const ccl::stream::impl_value_t& stream,
                                                      const ccl::broadcast_attr& attr,
                                                      const ccl::vector_class<ccl::event>& deps) {
    return {};
}

/* reduce */
ccl::event single_device_communicator::reduce_impl(const void* send_buf,
                                                   void* recv_buf,
                                                   size_t count,
                                                   ccl::datatype dtype,
                                                   ccl::reduction reduction,
                                                   int root,

                                                   const ccl::stream::impl_value_t& stream,
                                                   const ccl::reduce_attr& attr,
                                                   const ccl::vector_class<ccl::event>& deps) {
    return {};
}

/* reduce_scatter */
ccl::event single_device_communicator::reduce_scatter_impl(
    const void* send_buf,
    void* recv_buf,
    size_t recv_count,
    ccl::datatype dtype,
    ccl::reduction reduction,

    const ccl::stream::impl_value_t& stream,
    const ccl::reduce_scatter_attr& attr,
    const ccl::vector_class<ccl::event>& deps) {
    return {};
}

/* sparse_allreduce */
ccl::event single_device_communicator::sparse_allreduce_impl(
    const void* send_ind_buf,
    size_t send_ind_count,
    const void* send_val_buf,
    size_t send_val_count,
    void* recv_ind_buf,
    size_t recv_ind_count,
    void* recv_val_buf,
    size_t recv_val_count,
    ccl::datatype index_dtype,
    ccl::datatype value_dtype,
    ccl::reduction reduction,

    const ccl::stream::impl_value_t& stream,
    const ccl::sparse_allreduce_attr& attr,
    const ccl::vector_class<ccl::event>& deps) {
    return {};
}

/* allgatherv */

template <class buffer_type>
ccl::event single_device_communicator::allgatherv_base_impl(
    const buffer_type* send_buf,
    size_t send_count,
    buffer_type* recv_buf,
    const ccl::vector_class<size_t>& recv_counts,
    const ccl::stream::impl_value_t& stream,
    const ccl_coll_attr& attr,
    const ccl::vector_class<ccl::event>& deps) {
    return {};
}
template <class buffer_type>
ccl::event single_device_communicator::allgatherv_impl(const buffer_type* send_buf,
                                                       size_t send_count,
                                                       buffer_type* recv_buf,
                                                       const ccl::vector_class<size_t>& recv_counts,
                                                       const ccl::stream::impl_value_t& stream,
                                                       const ccl::allgatherv_attr& attr,
                                                       const ccl::vector_class<ccl::event>& deps) {
    return {};
}
template <class buffer_type>
ccl::event single_device_communicator::allgatherv_impl(const buffer_type* send_buf,
                                                       size_t send_count,
                                                       ccl::vector_class<buffer_type*>& recv_buf,
                                                       const ccl::vector_class<size_t>& recv_counts,
                                                       const ccl::stream::impl_value_t& stream,
                                                       const ccl::allgatherv_attr& attr,

                                                       const ccl::vector_class<ccl::event>& deps) {
    return {};
}

template <class buffer_type>
ccl::event single_device_communicator::allgatherv_impl(const buffer_type& send_buf,
                                                       size_t send_count,
                                                       buffer_type& recv_buf,
                                                       const ccl::vector_class<size_t>& recv_counts,
                                                       const ccl::stream::impl_value_t& stream,
                                                       const ccl::allgatherv_attr& attr,
                                                       const ccl::vector_class<ccl::event>& deps) {
    return {};
}
template <class buffer_type>
ccl::event single_device_communicator::allgatherv_impl(
    const buffer_type& send_buf,
    size_t send_count,
    ccl::vector_class<ccl::reference_wrapper_class<buffer_type>>& recv_buf,
    const ccl::vector_class<size_t>& recv_counts,
    const ccl::stream::impl_value_t& stream,
    const ccl::allgatherv_attr& attr,

    const ccl::vector_class<ccl::event>& deps) {
    return {};
}

/* allreduce */
template <class buffer_type>
ccl::event single_device_communicator::allreduce_impl(const buffer_type* send_buf,
                                                      buffer_type* recv_buf,
                                                      size_t count,
                                                      ccl::reduction reduction,
                                                      const ccl::stream::impl_value_t& stream,
                                                      const ccl::allreduce_attr& attr,
                                                      const ccl::vector_class<ccl::event>& deps) {
    return {};
}

template <class buffer_type>
ccl::event single_device_communicator::allreduce_impl(const buffer_type& send_buf,
                                                      buffer_type& recv_buf,
                                                      size_t count,
                                                      ccl::reduction reduction,
                                                      const ccl::stream::impl_value_t& stream,
                                                      const ccl::allreduce_attr& attr,
                                                      const ccl::vector_class<ccl::event>& deps) {
    return {};
}

/* alltoall */
template <class buffer_type>
ccl::event single_device_communicator::alltoall_impl(const buffer_type* send_buf,
                                                     buffer_type* recv_buf,
                                                     size_t count,
                                                     const ccl::stream::impl_value_t& stream,
                                                     const ccl::alltoall_attr& attr,
                                                     const ccl::vector_class<ccl::event>& deps) {
    return {};
}
template <class buffer_type>
ccl::event single_device_communicator::alltoall_impl(
    const ccl::vector_class<buffer_type*>& send_buf,
    const ccl::vector_class<buffer_type*>& recv_buf,
    size_t count,
    const ccl::stream::impl_value_t& stream,
    const ccl::alltoall_attr& attr,

    const ccl::vector_class<ccl::event>& deps) {
    return {};
}

template <class buffer_type>
ccl::event single_device_communicator::alltoall_impl(const buffer_type& send_buf,
                                                     buffer_type& recv_buf,
                                                     size_t count,
                                                     const ccl::stream::impl_value_t& stream,
                                                     const ccl::alltoall_attr& attr,
                                                     const ccl::vector_class<ccl::event>& deps) {
    return {};
}
template <class buffer_type>
ccl::event single_device_communicator::alltoall_impl(
    const ccl::vector_class<ccl::reference_wrapper_class<buffer_type>>& send_buf,
    const ccl::vector_class<ccl::reference_wrapper_class<buffer_type>>& recv_buf,
    size_t count,
    const ccl::stream::impl_value_t& stream,
    const ccl::alltoall_attr& attr,

    const ccl::vector_class<ccl::event>& dep) {
    return {};
}

/* alltoallv */
template <class buffer_type>
ccl::event single_device_communicator::alltoallv_impl(const buffer_type* send_buf,
                                                      const ccl::vector_class<size_t>& send_counts,
                                                      buffer_type* recv_buf,
                                                      const ccl::vector_class<size_t>& recv_counts,
                                                      const ccl::stream::impl_value_t& stream,
                                                      const ccl::alltoallv_attr& attr,
                                                      const ccl::vector_class<ccl::event>& deps) {
    return {};
}
template <class buffer_type>
ccl::event single_device_communicator::alltoallv_impl(
    const ccl::vector_class<buffer_type*>& send_buf,
    const ccl::vector_class<size_t>& send_counts,
    const ccl::vector_class<buffer_type*>& recv_buf,
    const ccl::vector_class<size_t>& recv_counts,
    const ccl::stream::impl_value_t& stream,
    const ccl::alltoallv_attr& attr,

    const ccl::vector_class<ccl::event>& dep) {
    return {};
}

template <class buffer_type>
ccl::event single_device_communicator::alltoallv_impl(const buffer_type& send_buf,
                                                      const ccl::vector_class<size_t>& send_counts,
                                                      buffer_type& recv_buf,
                                                      const ccl::vector_class<size_t>& recv_counts,
                                                      const ccl::stream::impl_value_t& stream,
                                                      const ccl::alltoallv_attr& attr,
                                                      const ccl::vector_class<ccl::event>& deps) {
    return {};
}
template <class buffer_type>
ccl::event single_device_communicator::alltoallv_impl(
    const ccl::vector_class<ccl::reference_wrapper_class<buffer_type>>& send_buf,
    const ccl::vector_class<size_t>& send_counts,
    const ccl::vector_class<ccl::reference_wrapper_class<buffer_type>>& recv_buf,
    const ccl::vector_class<size_t>& recv_counts,
    const ccl::stream::impl_value_t& stream,
    const ccl::alltoallv_attr& attr,

    const ccl::vector_class<ccl::event>& dep) {
    return {};
}

/* bcast */
template <class buffer_type>
ccl::event single_device_communicator::broadcast_impl(buffer_type* buf,
                                                      size_t count,
                                                      int root,
                                                      const ccl::stream::impl_value_t& stream,
                                                      const ccl::broadcast_attr& attr,
                                                      const ccl::vector_class<ccl::event>& deps) {
    return {};
}

template <class buffer_type>
ccl::event single_device_communicator::broadcast_impl(buffer_type& buf,
                                                      size_t count,
                                                      int root,
                                                      const ccl::stream::impl_value_t& stream,
                                                      const ccl::broadcast_attr& attr,
                                                      const ccl::vector_class<ccl::event>& deps) {
    return {};
}

/* reduce */
template <class buffer_type>
ccl::event single_device_communicator::reduce_impl(const buffer_type* send_buf,
                                                   buffer_type* recv_buf,
                                                   size_t count,
                                                   ccl::reduction reduction,
                                                   int root,
                                                   const ccl::stream::impl_value_t& stream,
                                                   const ccl::reduce_attr& attr,
                                                   const ccl::vector_class<ccl::event>& deps) {
    return {};
}

template <class buffer_type>
ccl::event single_device_communicator::reduce_impl(const buffer_type& send_buf,
                                                   buffer_type& recv_buf,
                                                   size_t count,
                                                   ccl::reduction reduction,
                                                   int root,
                                                   const ccl::stream::impl_value_t& stream,
                                                   const ccl::reduce_attr& attr,
                                                   const ccl::vector_class<ccl::event>& deps) {
    return {};
}
/* reduce_scatter */
template <class buffer_type>
ccl::event single_device_communicator::reduce_scatter_impl(
    const buffer_type* send_buf,
    buffer_type* recv_buf,
    size_t recv_count,
    ccl::reduction reduction,
    const ccl::stream::impl_value_t& stream,
    const ccl::reduce_scatter_attr& attr,
    const ccl::vector_class<ccl::event>& deps) {
    return {};
}
template <class buffer_type>
ccl::event single_device_communicator::reduce_scatter_impl(
    const buffer_type& send_buf,
    buffer_type& recv_buf,
    size_t recv_count,
    ccl::reduction reduction,
    const ccl::stream::impl_value_t& stream,
    const ccl::reduce_scatter_attr& attr,
    const ccl::vector_class<ccl::event>& deps) {
    return {};
}

/* sparse_allreduce */
template <class index_buffer_type, class value_buffer_type>
ccl::event single_device_communicator::sparse_allreduce_impl(
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
    return {};
}

template <class index_buffer_container_type, class value_buffer_container_type>
ccl::event single_device_communicator::sparse_allreduce_impl(
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
    return {};
}

COMM_INTERFACE_COLL_INSTANTIATION(single_device_communicator);
#ifdef CCL_ENABLE_SYCL
SYCL_COMM_INTERFACE_COLL_INSTANTIATION(single_device_communicator);
#endif // CCL_ENABLE_SYCL
