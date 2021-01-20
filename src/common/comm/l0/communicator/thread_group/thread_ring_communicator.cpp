#include "oneapi/ccl.hpp"
#include "oneapi/ccl/type_traits.hpp"
#include "common/comm/l0/communicator/thread_group/thread_ring_communicator_impl.hpp"
#include "common/comm/l0/gpu_comm_attr.hpp"
#include "common/comm/l0/context/process_group_ctx.hpp"

using namespace ccl;

thread_device_group_ring_communicator::thread_device_group_ring_communicator(
    ccl::unified_device_type&& device,
    ccl::unified_context_type&& ctx,
    size_t thread_idx,
    size_t process_idx,
    const ccl::comm_split_attr& attr)
        : base_t(std::move(device), std::move(ctx), thread_idx, process_idx, /*comm_attr, */ attr) {
}

void thread_device_group_ring_communicator::visit(ccl::gpu_comm_attr& comm_attr) {
    auto process_ctx = comm_attr.get_process_context();
    auto thread_ctx = process_ctx->get_thread_context(process_id);
    auto device_ctx = thread_ctx->get_device_group_ctx(thread_id);
    (void)device_ctx;

    ctx = thread_ctx;

    //get rank & size
    auto topology = ctx->get_thread_topology<base_t::topology_class()>(thread_id);
    this->initialize_comm_addr(get_device_path(), topology);

    this->set_comm_group_id(comm_attr.get_unique_id());
}

/*
size_t thread_device_group_ring_communicator::group_size() const
{
    return get_device_count<l0::ccl_gpu_comm>() +
              / * get_device_count<ccl_concurrent<SOURCE!!!>>() + Will add further* /
            get_device_count<l0::ccl_virtual_gpu_comm>();

}
*/
ccl::event thread_device_group_ring_communicator::barrier(
    const ccl::stream::impl_value_t& stream,
    const ccl::barrier_attr& attr,
    const ccl::vector_class<ccl::event>& deps) {
    throw ccl::exception(std::string(__PRETTY_FUNCTION__) + " - is not implemented yet");
}

/* allgatherv */
ccl::event thread_device_group_ring_communicator::allgatherv_impl(
    const void* send_buf,
    size_t send_count,
    void* recv_buf,
    const ccl::vector_class<size_t>& recv_counts,
    ccl::datatype dtype,
    const ccl::stream::impl_value_t& stream,
    const ccl::allgatherv_attr& attr,
    const ccl::vector_class<ccl::event>& deps) {
    ccl::event req;
    allgather_visitor_t::visit(
        req, dtype, send_buf, send_count, recv_buf, recv_counts, stream, attr, deps);
    return req;
}
ccl::event thread_device_group_ring_communicator::allgatherv_impl(
    const void* send_buf,
    size_t send_count,
    const ccl::vector_class<void*>& recv_bufs,
    const ccl::vector_class<size_t>& recv_counts,
    ccl::datatype dtype,
    const ccl::stream::impl_value_t& stream,
    const ccl::allgatherv_attr& attr,

    const ccl::vector_class<ccl::event>& deps) {
    throw ccl::exception(std::string(__PRETTY_FUNCTION__) + " - is not implemented");
    return {};
}

/* allreduce */
ccl::event thread_device_group_ring_communicator::allreduce_impl(
    const void* send_buf,
    void* recv_buf,
    size_t count,
    ccl::datatype dtype,
    ccl::reduction reduction,
    const ccl::stream::impl_value_t& stream,
    const ccl::allreduce_attr& attr,
    const ccl::vector_class<ccl::event>& deps) {
    ccl::event req;
    allreduce_visitor_t::visit(
        req, dtype, send_buf, recv_buf, count, reduction, stream, attr, deps);
    return req;
}

/* alltoall */
ccl::event thread_device_group_ring_communicator::alltoall_impl(
    const void* send_buf,
    void* recv_buf,
    size_t count,
    ccl::datatype dtype,
    const ccl::stream::impl_value_t& stream,
    const ccl::alltoall_attr& attr,
    const ccl::vector_class<ccl::event>& deps) {
    ccl::event req;
    alltoall_visitor_t::visit(req, dtype, send_buf, recv_buf, count, stream, attr, deps);
    return req;
}
ccl::event thread_device_group_ring_communicator::alltoall_impl(
    const ccl::vector_class<void*>& send_buf,
    const ccl::vector_class<void*>& recv_buf,
    size_t count,
    ccl::datatype dtype,
    const ccl::stream::impl_value_t& stream,
    const ccl::alltoall_attr& attr,
    const ccl::vector_class<ccl::event>& deps) {
    throw ccl::exception(std::string(__PRETTY_FUNCTION__) + " - is not implemented");
    return {};
}

/* alltoallv */
ccl::event thread_device_group_ring_communicator::alltoallv_impl(
    const void* send_buf,
    const ccl::vector_class<size_t>& send_counts,
    void* recv_buf,
    const ccl::vector_class<size_t>& recv_counts,
    ccl::datatype dtype,
    const ccl::stream::impl_value_t& stream,
    const ccl::alltoallv_attr& attr,
    const ccl::vector_class<ccl::event>& deps) {
    ccl::event req;
    alltoallv_visitor_t::visit(
        req, dtype, send_buf, send_counts, recv_buf, recv_counts, stream, attr, deps);
    return req;
}
ccl::event thread_device_group_ring_communicator::alltoallv_impl(
    const ccl::vector_class<void*>& send_buf,
    const ccl::vector_class<size_t>& send_counts,
    ccl::vector_class<void*> recv_buf,
    const ccl::vector_class<size_t>& recv_counts,
    ccl::datatype dtype,
    const ccl::stream::impl_value_t& stream,
    const ccl::alltoallv_attr& attr,

    const ccl::vector_class<ccl::event>& dep) {
    throw ccl::exception(std::string(__PRETTY_FUNCTION__) + " - is not implemented");
    return {};
}

/* bcast */
ccl::event thread_device_group_ring_communicator::broadcast_impl(
    void* buf,
    size_t count,
    ccl::datatype dtype,
    int root,
    const ccl::stream::impl_value_t& stream,
    const ccl::broadcast_attr& attr,
    const ccl::vector_class<ccl::event>& deps) {
    ccl::event req;
    broadcast_visitor_t::visit(req, dtype, buf, count, root, stream, attr, deps);
    return req;
}

/* reduce */
ccl::event thread_device_group_ring_communicator::reduce_impl(
    const void* send_buf,
    void* recv_buf,
    size_t count,
    ccl::datatype dtype,
    ccl::reduction reduction,
    int root,
    const ccl::stream::impl_value_t& stream,
    const ccl::reduce_attr& attr,
    const ccl::vector_class<ccl::event>& deps) {
    ccl::event req;
    reduce_visitor_t::visit(
        req, dtype, send_buf, recv_buf, count, reduction, root, stream, attr, deps);
    return req;
}

/* reduce_scatter */
ccl::event thread_device_group_ring_communicator::reduce_scatter_impl(
    const void* send_buf,
    void* recv_buf,
    size_t recv_count,
    ccl::datatype dtype,
    ccl::reduction reduction,
    const ccl::stream::impl_value_t& stream,
    const ccl::reduce_scatter_attr& attr,
    const ccl::vector_class<ccl::event>& deps) {
    ccl::event req;
    reduce_scatter_visitor_t::visit(
        req, dtype, send_buf, recv_buf, recv_count, reduction, stream, attr, deps);
    return req;
}

/* sparse_allreduce */
ccl::event thread_device_group_ring_communicator::sparse_allreduce_impl(
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
    throw ccl::exception(std::string(__PRETTY_FUNCTION__) + " - is not implemented");
    return {};
}

COMM_INTERFACE_COLL_INSTANTIATION(thread_device_group_ring_communicator);
#ifdef CCL_ENABLE_SYCL
SYCL_COMM_INTERFACE_COLL_INSTANTIATION(thread_device_group_ring_communicator);
#endif /* CCL_ENABLE_SYCL */
