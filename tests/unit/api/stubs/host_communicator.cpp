#include "common/comm/host_communicator/host_communicator.hpp"
#include "oneapi/ccl/ccl_types.hpp"
#include "oneapi/ccl/ccl_type_traits.hpp"
#include "oneapi/ccl/ccl_types_policy.hpp"
#include "oneapi/ccl/ccl_coll_attr_ids.hpp"
#include "oneapi/ccl/ccl_coll_attr_ids_traits.hpp"
#include "oneapi/ccl/ccl_coll_attr.hpp"

#include "oneapi/ccl/ccl_event.hpp"
#include "common/global/global.hpp"

#ifdef MULTI_GPU_SUPPORT
#include "common/comm/l0/gpu_comm_attr.hpp"
#endif

namespace ccl {

host_communicator::host_communicator()
        : comm_attr(ccl::ccl_empty_attr{})
{
}

host_communicator::host_communicator(size_t size, shared_ptr_class<kvs_interface> kvs)
        : comm_attr(ccl::ccl_empty_attr{}),
          comm_rank(0),
          comm_size(size) {
    if (size <= 0) {
        throw ccl::exception("Incorrect size value when creating a host communicator");
    }
}

host_communicator::host_communicator(size_t size, size_t rank, shared_ptr_class<kvs_interface> kvs)
        : comm_attr(ccl::ccl_empty_attr{}),
          comm_rank(rank),
          comm_size(size) {
}

host_communicator::host_communicator(std::shared_ptr<atl_wrapper> atl)
        : comm_attr(ccl::ccl_empty_attr{}),
          comm_rank(0),
          comm_size(0) {
}

host_communicator::host_communicator(std::shared_ptr<ccl_comm> impl)
        : comm_impl(impl),
          comm_attr(ccl::ccl_empty_attr{}),
          comm_rank(impl->rank()),
          comm_size(impl->size()) {}

size_t host_communicator::rank() const {
    return comm_rank;
}

size_t host_communicator::size() const {
    return comm_size;
}

#ifdef MULTI_GPU_SUPPORT
void host_communicator::visit(ccl::gpu_comm_attr& comm_attr) {
    (void)(comm_attr);
}
#endif

ccl::device_index_type host_communicator::get_device_path() const {
    return ccl::device_index_type{ ccl::unused_index_value,
                                   ccl::unused_index_value,
                                   ccl::unused_index_value };
}

ccl::communicator_interface::device_t host_communicator::get_device() {
    throw ccl::exception(std::string(__FUNCTION__) + " is not applicable for " + traits::name());
}

ccl::communicator_interface::context_t host_communicator::get_context() {
    throw ccl::exception(std::string(__FUNCTION__) + " is not applicable for " + traits::name());
}

ccl::communicator_interface_ptr host_communicator::split(const comm_split_attr& attr) {
    return {};
}

host_communicator::coll_request_t host_communicator::barrier(
    const ccl::stream::impl_value_t& op_stream,
    const ccl::barrier_attr& attr,
    const ccl::vector_class<ccl::event>& deps) {return {};
}

host_communicator::coll_request_t host_communicator::barrier_impl(
    const ccl::stream::impl_value_t& op_stream,
    const ccl::barrier_attr& attr,
    const ccl::vector_class<ccl::event>& deps) {
    return {};
}

/* allgatherv */
host_communicator::coll_request_t host_communicator::allgatherv_impl(
    const void* send_buf,
    size_t send_count,
    void* recv_buf,
    const ccl::vector_class<size_t>& recv_counts,
    ccl::datatype dtype,
    const ccl::stream::impl_value_t& stream,
    const ccl::allgatherv_attr& attr,
    const ccl::vector_class<ccl::event>& deps) {
    return {};
}

host_communicator::coll_request_t host_communicator::allgatherv_impl(
    const void* send_buf,
    size_t send_count,
    const ccl::vector_class<void*>& recv_bufs,
    const ccl::vector_class<size_t>& recv_counts,
    ccl::datatype dtype,
    const ccl::stream::impl_value_t& stream,
    const ccl::allgatherv_attr& attr,
    const ccl::vector_class<ccl::event>& deps) {
    // TODO not implemented
    throw ccl::exception(std::string(__PRETTY_FUNCTION__) + " - is not implemented");
    return {};
}

/* allreduce */
host_communicator::coll_request_t host_communicator::allreduce_impl(
    const void* send_buf,
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
host_communicator::coll_request_t host_communicator::alltoall_impl(
    const void* send_buf,
    void* recv_buf,
    size_t count,
    ccl::datatype dtype,
    const ccl::stream::impl_value_t& stream,
    const ccl::alltoall_attr& attr,
    const ccl::vector_class<ccl::event>& deps) {return {};
}

host_communicator::coll_request_t host_communicator::alltoall_impl(
    const ccl::vector_class<void*>& send_buf,
    const ccl::vector_class<void*>& recv_buf,
    size_t count,
    ccl::datatype dtype,
    const ccl::stream::impl_value_t& stream,
    const ccl::alltoall_attr& attr,
    const ccl::vector_class<ccl::event>& deps) {
        return {};
}

/* alltoallv */
host_communicator::coll_request_t host_communicator::alltoallv_impl(
    const void* send_buf,
    const ccl::vector_class<size_t>& send_counts,
    void* recv_buf,
    const ccl::vector_class<size_t>& recv_counts,
    ccl::datatype dtype,
    const ccl::stream::impl_value_t& stream,
    const ccl::alltoallv_attr& attr,
    const ccl::vector_class<ccl::event>& deps) {return {};
}

host_communicator::coll_request_t host_communicator::alltoallv_impl(
    const ccl::vector_class<void*>& send_buf,
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
host_communicator::coll_request_t host_communicator::broadcast_impl(
    void* buf,
    size_t count,
    ccl::datatype dtype,
    size_t root,
    const ccl::stream::impl_value_t& stream,
    const ccl::broadcast_attr& attr,
    const ccl::vector_class<ccl::event>& deps) {return {};
}

/* reduce */
host_communicator::coll_request_t host_communicator::reduce_impl(
    const void* send_buf,
    void* recv_buf,
    size_t count,
    ccl::datatype dtype,
    ccl::reduction reduction,
    size_t root,
    const ccl::stream::impl_value_t& stream,
    const ccl::reduce_attr& attr,
    const ccl::vector_class<ccl::event>& deps) {return {};
}

/* reduce_scatter */
host_communicator::coll_request_t host_communicator::reduce_scatter_impl(
    const void* send_buf,
    void* recv_buf,
    size_t recv_count,
    ccl::datatype dtype,
    ccl::reduction reduction,
    const ccl::stream::impl_value_t& stream,
    const ccl::reduce_scatter_attr& attr,
    const ccl::vector_class<ccl::event>& deps) {return {};
}

/* sparse_allreduce */
host_communicator::coll_request_t host_communicator::sparse_allreduce_impl(
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
    const ccl::vector_class<ccl::event>& deps) {return {};
}

/* allgatherv */
template <class buffer_type>
host_communicator::coll_request_t host_communicator::allgatherv_impl(
    const buffer_type* send_buf,
    size_t send_count,
    buffer_type* recv_buf,
    const ccl::vector_class<size_t>& recv_counts,
    const ccl::stream::impl_value_t& stream,
    const ccl::allgatherv_attr& attr,
    const ccl::vector_class<ccl::event>& deps) {
    return {};
}
template <class buffer_type>
host_communicator::coll_request_t host_communicator::allgatherv_impl(
    const buffer_type* send_buf,
    size_t send_count,
    ccl::vector_class<buffer_type*>& recv_buf,
    const ccl::vector_class<size_t>& recv_counts,
    const ccl::stream::impl_value_t& stream,
    const ccl::allgatherv_attr& attr,

    const ccl::vector_class<ccl::event>& deps) {
    return {};
}

template <class buffer_type>
host_communicator::coll_request_t host_communicator::allgatherv_impl(
    const buffer_type& send_buf,
    size_t send_count,
    buffer_type& recv_buf,
    const ccl::vector_class<size_t>& recv_counts,
    const ccl::stream::impl_value_t& stream,
    const ccl::allgatherv_attr& attr,
    const ccl::vector_class<ccl::event>& deps) {
    return {};
}
template <class buffer_type>
host_communicator::coll_request_t host_communicator::allgatherv_impl(
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
host_communicator::coll_request_t host_communicator::allreduce_impl(
    const buffer_type* send_buf,
    buffer_type* recv_buf,
    size_t count,
    ccl::reduction reduction,
    const ccl::stream::impl_value_t& stream,
    const ccl::allreduce_attr& attr,
    const ccl::vector_class<ccl::event>& deps) {
    return {};
}

template <class buffer_type>
host_communicator::coll_request_t host_communicator::allreduce_impl(
    const buffer_type& send_buf,
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
host_communicator::coll_request_t host_communicator::alltoall_impl(
    const buffer_type* send_buf,
    buffer_type* recv_buf,
    size_t count,
    const ccl::stream::impl_value_t& stream,
    const ccl::alltoall_attr& attr,
    const ccl::vector_class<ccl::event>& deps) {
    return {};
}
template <class buffer_type>
host_communicator::coll_request_t host_communicator::alltoall_impl(
    const ccl::vector_class<buffer_type*>& send_buf,
    const ccl::vector_class<buffer_type*>& recv_buf,
    size_t count,
    const ccl::stream::impl_value_t& stream,
    const ccl::alltoall_attr& attr,

    const ccl::vector_class<ccl::event>& deps) {
    return {};
}

template <class buffer_type>
host_communicator::coll_request_t host_communicator::alltoall_impl(
    const buffer_type& send_buf,
    buffer_type& recv_buf,
    size_t count,
    const ccl::stream::impl_value_t& stream,
    const ccl::alltoall_attr& attr,
    const ccl::vector_class<ccl::event>& deps) {
    return {};
}
template <class buffer_type>
host_communicator::coll_request_t host_communicator::alltoall_impl(
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
host_communicator::coll_request_t host_communicator::alltoallv_impl(
    const buffer_type* send_buf,
    const ccl::vector_class<size_t>& send_counts,
    buffer_type* recv_buf,
    const ccl::vector_class<size_t>& recv_counts,
    const ccl::stream::impl_value_t& stream,
    const ccl::alltoallv_attr& attr,
    const ccl::vector_class<ccl::event>& deps) {
    return {};
}
template <class buffer_type>
host_communicator::coll_request_t host_communicator::alltoallv_impl(
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
host_communicator::coll_request_t host_communicator::alltoallv_impl(
    const buffer_type& send_buf,
    const ccl::vector_class<size_t>& send_counts,
    buffer_type& recv_buf,
    const ccl::vector_class<size_t>& recv_counts,
    const ccl::stream::impl_value_t& stream,
    const ccl::alltoallv_attr& attr,
    const ccl::vector_class<ccl::event>& deps) {
    return {};
}
template <class buffer_type>
host_communicator::coll_request_t host_communicator::alltoallv_impl(
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
host_communicator::coll_request_t host_communicator::broadcast_impl(
    buffer_type* buf,
    size_t count,
    size_t root,
    const ccl::stream::impl_value_t& stream,
    const ccl::broadcast_attr& attr,
    const ccl::vector_class<ccl::event>& deps) {
    return {};
}

template <class buffer_type>
host_communicator::coll_request_t host_communicator::broadcast_impl(
    buffer_type& buf,
    size_t count,
    size_t root,
    const ccl::stream::impl_value_t& stream,
    const ccl::broadcast_attr& attr,
    const ccl::vector_class<ccl::event>& deps) {
    return {};
}

/* reduce */
template <class buffer_type>
host_communicator::coll_request_t host_communicator::reduce_impl(
    const buffer_type* send_buf,
    buffer_type* recv_buf,
    size_t count,
    ccl::reduction reduction,
    size_t root,
    const ccl::stream::impl_value_t& stream,
    const ccl::reduce_attr& attr,
    const ccl::vector_class<ccl::event>& deps) {
    return {};
}

template <class buffer_type>
host_communicator::coll_request_t host_communicator::reduce_impl(
    const buffer_type& send_buf,
    buffer_type& recv_buf,
    size_t count,
    ccl::reduction reduction,
    size_t root,
    const ccl::stream::impl_value_t& stream,
    const ccl::reduce_attr& attr,
    const ccl::vector_class<ccl::event>& deps) {
    return {};
}
/* reduce_scatter */
template <class buffer_type>
host_communicator::coll_request_t host_communicator::reduce_scatter_impl(
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
host_communicator::coll_request_t host_communicator::reduce_scatter_impl(
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
host_communicator::coll_request_t host_communicator::sparse_allreduce_impl(
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
host_communicator::coll_request_t host_communicator::sparse_allreduce_impl(
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
std::shared_ptr<atl_wrapper> host_communicator::get_atl() {
    return comm_impl->atl;
}

std::string host_communicator::to_string() const {return {};
}

DEVICE_COMM_INTERFACE_COLL_INSTANTIATIONS(host_communicator, char);
DEVICE_COMM_INTERFACE_COLL_INSTANTIATIONS(host_communicator, int);
DEVICE_COMM_INTERFACE_COLL_INSTANTIATIONS(host_communicator, int64_t);
DEVICE_COMM_INTERFACE_COLL_INSTANTIATIONS(host_communicator, uint64_t);
DEVICE_COMM_INTERFACE_COLL_INSTANTIATIONS(host_communicator, float);
DEVICE_COMM_INTERFACE_COLL_INSTANTIATIONS(host_communicator, double);

#ifdef CCL_ENABLE_SYCL
DEVICE_COMM_INTERFACE_COLL_CLASS_INSTANTIATIONS(host_communicator,
                                                cl::sycl::buffer<char COMMA 1>);
DEVICE_COMM_INTERFACE_COLL_CLASS_INSTANTIATIONS(host_communicator,
                                                cl::sycl::buffer<int COMMA 1>);
DEVICE_COMM_INTERFACE_COLL_CLASS_INSTANTIATIONS(host_communicator,
                                                cl::sycl::buffer<int64_t COMMA 1>);
DEVICE_COMM_INTERFACE_COLL_CLASS_INSTANTIATIONS(host_communicator,
                                                cl::sycl::buffer<uint64_t COMMA 1>);
DEVICE_COMM_INTERFACE_COLL_CLASS_INSTANTIATIONS(host_communicator,
                                                cl::sycl::buffer<float COMMA 1>);
DEVICE_COMM_INTERFACE_COLL_CLASS_INSTANTIATIONS(host_communicator,
                                                cl::sycl::buffer<double COMMA 1>);
#endif //CCL_ENABLE_SYCL

DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(host_communicator,
                                                              char,
                                                              char);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(host_communicator,
                                                              char,
                                                              int);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(host_communicator,
                                                              char,
                                                              ccl::bf16);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(host_communicator,
                                                              char,
                                                              float);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(host_communicator,
                                                              char,
                                                              double);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(host_communicator,
                                                              char,
                                                              int64_t);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(host_communicator,
                                                              char,
                                                              uint64_t);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(host_communicator,
                                                              int,
                                                              char);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(host_communicator, int, int);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(host_communicator,
                                                              int,
                                                              ccl::bf16);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(host_communicator,
                                                              int,
                                                              float);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(host_communicator,
                                                              int,
                                                              double);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(host_communicator,
                                                              int,
                                                              int64_t);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(host_communicator,
                                                              int,
                                                              uint64_t);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(host_communicator,
                                                              int64_t,
                                                              char);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(host_communicator,
                                                              int64_t,
                                                              int);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(host_communicator,
                                                              int64_t,
                                                              ccl::bf16);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(host_communicator,
                                                              int64_t,
                                                              float);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(host_communicator,
                                                              int64_t,
                                                              double);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(host_communicator,
                                                              int64_t,
                                                              int64_t);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(host_communicator,
                                                              int64_t,
                                                              uint64_t);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(host_communicator,
                                                              uint64_t,
                                                              char);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(host_communicator,
                                                              uint64_t,
                                                              int);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(host_communicator,
                                                              uint64_t,
                                                              ccl::bf16);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(host_communicator,
                                                              uint64_t,
                                                              float);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(host_communicator,
                                                              uint64_t,
                                                              double);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(host_communicator,
                                                              uint64_t,
                                                              int64_t);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(host_communicator,
                                                              uint64_t,
                                                              uint64_t);

#ifdef CCL_ENABLE_SYCL
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_CLASS_INSTANTIATION(
    host_communicator,
    cl::sycl::buffer<int COMMA 1>,
    cl::sycl::buffer<float COMMA 1>);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_CLASS_INSTANTIATION(
    host_communicator,
    cl::sycl::buffer<int COMMA 1>,
    cl::sycl::buffer<ccl::bf16 COMMA 1>);

DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_CLASS_INSTANTIATION(
    host_communicator,
    cl::sycl::buffer<int64_t COMMA 1>,
    cl::sycl::buffer<float COMMA 1>);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_CLASS_INSTANTIATION(
    host_communicator,
    cl::sycl::buffer<int64_t COMMA 1>,
    cl::sycl::buffer<ccl::bf16 COMMA 1>);
#endif //CCL_ENABLE_SYCL

} // namespace ccl
