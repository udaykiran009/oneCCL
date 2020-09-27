#include "oneapi/ccl.hpp"
#include "oneapi/ccl/ccl_type_traits.hpp"
#include "common/comm/l0/communicator/thread_group/thread_ring_communicator_impl.hpp"
#include "common/comm/l0/gpu_comm_attr.hpp"
#include "common/comm/l0/context/process_group_ctx.hpp"

using namespace ccl;

thread_device_group_ring_communicator::thread_device_group_ring_communicator(
    ccl::unified_device_type&& device,
    size_t thread_idx,
    size_t process_idx,
    const ccl::device_comm_split_attr& attr)
        : base_t(std::move(device), thread_idx, process_idx, /*comm_attr, */ attr) {}

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
thread_device_group_ring_communicator::coll_request_t
thread_device_group_ring_communicator::barrier(ccl::stream::impl_value_t& stream,
                                               const ccl::barrier_attr& attr,
                                               const ccl::vector_class<ccl::event>& deps) {
    throw ccl::ccl_error(std::string(__PRETTY_FUNCTION__) + " - is not implemented yet");
}

/* allgatherv */
thread_device_group_ring_communicator::coll_request_t
thread_device_group_ring_communicator::allgatherv_impl(const void* send_buf,
                                                       size_t send_count,
                                                       void* recv_buf,
                                                       const ccl::vector_class<size_t>& recv_counts,
                                                       ccl::datatype dtype,
                                                       ccl::stream::impl_value_t& stream,
                                                       const ccl::allgatherv_attr& attr,
                                                       const ccl::vector_class<ccl::event>& deps) {
    throw ccl::ccl_error(std::string(__PRETTY_FUNCTION__) + " - is not implemented");
    return {};
}
thread_device_group_ring_communicator::coll_request_t
thread_device_group_ring_communicator::allgatherv_impl(const void* send_buf,
                                                       size_t send_count,
                                                       const ccl::vector_class<void*>& recv_bufs,
                                                       const ccl::vector_class<size_t>& recv_counts,
                                                       ccl::datatype dtype,
                                                       ccl::stream::impl_value_t& stream,
                                                       const ccl::allgatherv_attr& attr,

                                                       const ccl::vector_class<ccl::event>& deps) {
    throw ccl::ccl_error(std::string(__PRETTY_FUNCTION__) + " - is not implemented");
    return {};
}

/* allreduce */
thread_device_group_ring_communicator::coll_request_t
thread_device_group_ring_communicator::allreduce_impl(const void* send_buf,
                                                      void* recv_buf,
                                                      size_t count,
                                                      ccl::datatype dtype,
                                                      ccl::reduction reduction,
                                                      ccl::stream::impl_value_t& stream,
                                                      const ccl::allreduce_attr& attr,
                                                      const ccl::vector_class<ccl::event>& deps) {
    throw ccl::ccl_error(std::string(__PRETTY_FUNCTION__) + " - is not implemented");
    return {};
}

/* alltoall */
thread_device_group_ring_communicator::coll_request_t
thread_device_group_ring_communicator::alltoall_impl(const void* send_buf,
                                                     void* recv_buf,
                                                     size_t count,
                                                     ccl::datatype dtype,
                                                     ccl::stream::impl_value_t& stream,
                                                     const ccl::alltoall_attr& attr,
                                                     const ccl::vector_class<ccl::event>& deps) {
    throw ccl::ccl_error(std::string(__PRETTY_FUNCTION__) + " - is not implemented");
    return {};
}
thread_device_group_ring_communicator::coll_request_t
thread_device_group_ring_communicator::alltoall_impl(const ccl::vector_class<void*>& send_buf,
                                                     const ccl::vector_class<void*>& recv_buf,
                                                     size_t count,
                                                     ccl::datatype dtype,
                                                     ccl::stream::impl_value_t& stream,
                                                     const ccl::alltoall_attr& attr,
                                                     const ccl::vector_class<ccl::event>& deps) {
    throw ccl::ccl_error(std::string(__PRETTY_FUNCTION__) + " - is not implemented");
    return {};
}

/* alltoallv */
thread_device_group_ring_communicator::coll_request_t
thread_device_group_ring_communicator::alltoallv_impl(const void* send_buf,
                                                      const ccl::vector_class<size_t>& send_counts,
                                                      void* recv_buf,
                                                      const ccl::vector_class<size_t>& recv_counts,
                                                      ccl::datatype dtype,
                                                      ccl::stream::impl_value_t& stream,
                                                      const ccl::alltoallv_attr& attr,
                                                      const ccl::vector_class<ccl::event>& deps) {
    throw ccl::ccl_error(std::string(__PRETTY_FUNCTION__) + " - is not implemented");
    return {};
}
thread_device_group_ring_communicator::coll_request_t
thread_device_group_ring_communicator::alltoallv_impl(const ccl::vector_class<void*>& send_buf,
                                                      const ccl::vector_class<size_t>& send_counts,
                                                      ccl::vector_class<void*> recv_buf,
                                                      const ccl::vector_class<size_t>& recv_counts,
                                                      ccl::datatype dtype,
                                                      ccl::stream::impl_value_t& stream,
                                                      const ccl::alltoallv_attr& attr,

                                                      const ccl::vector_class<ccl::event>& dep) {
    throw ccl::ccl_error(std::string(__PRETTY_FUNCTION__) + " - is not implemented");
    return {};
}

/* bcast */
thread_device_group_ring_communicator::coll_request_t
thread_device_group_ring_communicator::broadcast_impl(void* buf,
                                                      size_t count,
                                                      ccl::datatype dtype,
                                                      size_t root,
                                                      ccl::stream::impl_value_t& stream,
                                                      const ccl::broadcast_attr& attr,
                                                      const ccl::vector_class<ccl::event>& deps) {
    throw ccl::ccl_error(std::string(__PRETTY_FUNCTION__) + " - is not implemented");
    return {};
}

/* reduce */
thread_device_group_ring_communicator::coll_request_t
thread_device_group_ring_communicator::reduce_impl(const void* send_buf,
                                                   void* recv_buf,
                                                   size_t count,
                                                   ccl::datatype dtype,
                                                   ccl::reduction reduction,
                                                   size_t root,
                                                   ccl::stream::impl_value_t& stream,
                                                   const ccl::reduce_attr& attr,
                                                   const ccl::vector_class<ccl::event>& deps) {
    throw ccl::ccl_error(std::string(__PRETTY_FUNCTION__) + " - is not implemented");
    return {};
}

/* reduce_scatter */
thread_device_group_ring_communicator::coll_request_t
thread_device_group_ring_communicator::reduce_scatter_impl(
    const void* send_buf,
    void* recv_buf,
    size_t recv_count,
    ccl::datatype dtype,
    ccl::reduction reduction,
    ccl::stream::impl_value_t& stream,
    const ccl::reduce_scatter_attr& attr,
    const ccl::vector_class<ccl::event>& deps) {
    throw ccl::ccl_error(std::string(__PRETTY_FUNCTION__) + " - is not implemented");
    return {};
}

/* sparse_allreduce */
thread_device_group_ring_communicator::coll_request_t
thread_device_group_ring_communicator::sparse_allreduce_impl(
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
    ccl::stream::impl_value_t& stream,
    const ccl::sparse_allreduce_attr& attr,
    const ccl::vector_class<ccl::event>& deps) {
    throw ccl::ccl_error(std::string(__PRETTY_FUNCTION__) + " - is not implemented");
    return {};
}

DEVICE_COMM_INTERFACE_COLL_INSTANTIATIONS(thread_device_group_ring_communicator, char);
DEVICE_COMM_INTERFACE_COLL_INSTANTIATIONS(thread_device_group_ring_communicator, int);
DEVICE_COMM_INTERFACE_COLL_INSTANTIATIONS(thread_device_group_ring_communicator, int64_t);
DEVICE_COMM_INTERFACE_COLL_INSTANTIATIONS(thread_device_group_ring_communicator, uint64_t);
DEVICE_COMM_INTERFACE_COLL_INSTANTIATIONS(thread_device_group_ring_communicator, float);
DEVICE_COMM_INTERFACE_COLL_INSTANTIATIONS(thread_device_group_ring_communicator, double);

#ifdef CCL_ENABLE_SYCL
DEVICE_COMM_INTERFACE_COLL_CLASS_INSTANTIATIONS(thread_device_group_ring_communicator,
                                                cl::sycl::buffer<char COMMA 1>);
DEVICE_COMM_INTERFACE_COLL_CLASS_INSTANTIATIONS(thread_device_group_ring_communicator,
                                                cl::sycl::buffer<int COMMA 1>);
DEVICE_COMM_INTERFACE_COLL_CLASS_INSTANTIATIONS(thread_device_group_ring_communicator,
                                                cl::sycl::buffer<int64_t COMMA 1>);
DEVICE_COMM_INTERFACE_COLL_CLASS_INSTANTIATIONS(thread_device_group_ring_communicator,
                                                cl::sycl::buffer<uint64_t COMMA 1>);
DEVICE_COMM_INTERFACE_COLL_CLASS_INSTANTIATIONS(thread_device_group_ring_communicator,
                                                cl::sycl::buffer<float COMMA 1>);
DEVICE_COMM_INTERFACE_COLL_CLASS_INSTANTIATIONS(thread_device_group_ring_communicator,
                                                cl::sycl::buffer<double COMMA 1>);
#endif //CCL_ENABLE_SYCL

DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(thread_device_group_ring_communicator,
                                                              char,
                                                              char);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(thread_device_group_ring_communicator,
                                                              char,
                                                              int);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(thread_device_group_ring_communicator,
                                                              char,
                                                              ccl::bf16);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(thread_device_group_ring_communicator,
                                                              char,
                                                              float);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(thread_device_group_ring_communicator,
                                                              char,
                                                              double);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(thread_device_group_ring_communicator,
                                                              char,
                                                              int64_t);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(thread_device_group_ring_communicator,
                                                              char,
                                                              uint64_t);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(thread_device_group_ring_communicator,
                                                              int,
                                                              char);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(thread_device_group_ring_communicator,
                                                              int,
                                                              int);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(thread_device_group_ring_communicator,
                                                              int,
                                                              ccl::bf16);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(thread_device_group_ring_communicator,
                                                              int,
                                                              float);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(thread_device_group_ring_communicator,
                                                              int,
                                                              double);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(thread_device_group_ring_communicator,
                                                              int,
                                                              int64_t);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(thread_device_group_ring_communicator,
                                                              int,
                                                              uint64_t);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(thread_device_group_ring_communicator,
                                                              int64_t,
                                                              char);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(thread_device_group_ring_communicator,
                                                              int64_t,
                                                              int);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(thread_device_group_ring_communicator,
                                                              int64_t,
                                                              ccl::bf16);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(thread_device_group_ring_communicator,
                                                              int64_t,
                                                              float);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(thread_device_group_ring_communicator,
                                                              int64_t,
                                                              double);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(thread_device_group_ring_communicator,
                                                              int64_t,
                                                              int64_t);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(thread_device_group_ring_communicator,
                                                              int64_t,
                                                              uint64_t);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(thread_device_group_ring_communicator,
                                                              uint64_t,
                                                              char);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(thread_device_group_ring_communicator,
                                                              uint64_t,
                                                              int);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(thread_device_group_ring_communicator,
                                                              uint64_t,
                                                              ccl::bf16);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(thread_device_group_ring_communicator,
                                                              uint64_t,
                                                              float);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(thread_device_group_ring_communicator,
                                                              uint64_t,
                                                              double);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(thread_device_group_ring_communicator,
                                                              uint64_t,
                                                              int64_t);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(thread_device_group_ring_communicator,
                                                              uint64_t,
                                                              uint64_t);

#ifdef CCL_ENABLE_SYCL
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_CLASS_INSTANTIATION(
    thread_device_group_ring_communicator,
    cl::sycl::buffer<int COMMA 1>,
    cl::sycl::buffer<float COMMA 1>);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_CLASS_INSTANTIATION(
    thread_device_group_ring_communicator,
    cl::sycl::buffer<int COMMA 1>,
    cl::sycl::buffer<ccl::bf16 COMMA 1>);

DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_CLASS_INSTANTIATION(
    thread_device_group_ring_communicator,
    cl::sycl::buffer<int64_t COMMA 1>,
    cl::sycl::buffer<float COMMA 1>);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_CLASS_INSTANTIATION(
    thread_device_group_ring_communicator,
    cl::sycl::buffer<int64_t COMMA 1>,
    cl::sycl::buffer<ccl::bf16 COMMA 1>);
#endif //CCL_ENABLE_SYCL
