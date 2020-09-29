#include "common/global/global.hpp"
#include "common/comm/host_communicator/host_communicator_impl.hpp"
#include "oneapi/ccl/ccl_comm_split_attr_ids.hpp"
#include "oneapi/ccl/ccl_comm_split_attr_ids_traits.hpp"
#include "oneapi/ccl/ccl_comm_split_attr.hpp"

#include "common/request/request.hpp"
#include "common/request/host_request.hpp"
#include "coll/coll.hpp"
#include "coll/coll_common_attributes.hpp"
#include "coll/ccl_allgather_op_attr.hpp"

#include "util/pm/pmi_resizable_rt/pmi_resizable/kvs/ikvs_wrapper.h"
#include "atl/atl_wrapper.h"

#ifdef MULTI_GPU_SUPPORT
#include "common/comm/l0/gpu_comm_attr.hpp"
#endif

namespace ccl {

host_communicator::host_communicator()
        : comm_attr(ccl::create_device_comm_split_attr())
{
}

host_communicator::host_communicator(size_t size, shared_ptr_class<kvs_interface> kvs)
        : comm_attr(ccl::create_device_comm_split_attr()),
          comm_rank(0),
          comm_size(size) {
    if (size <= 0) {
        throw ccl::exception("Incorrect size value when creating a host communicator");
    }
}

host_communicator::host_communicator(size_t size, size_t rank, shared_ptr_class<kvs_interface> kvs)
        : comm_attr(ccl::create_device_comm_split_attr()),
          comm_rank(rank),
          comm_size(size) {
    if (rank > size || size <= 0) {
        throw ccl::exception("Incorrect rank or size value when creating a host communicator");
    }

    ccl::global_data& data = ccl::global_data::get();
    std::shared_ptr<ikvs_wrapper> kvs_tmp = std::shared_ptr<ikvs_wrapper>(new users_kvs(kvs));
    std::shared_ptr<atl_wrapper> atl_tmp =
        std::shared_ptr<atl_wrapper>(new atl_wrapper(size, { rank }, kvs_tmp));
    comm_impl =
        std::shared_ptr<ccl_comm>(new ccl_comm(rank, size, data.comm_ids->acquire(), atl_tmp));
}

host_communicator::host_communicator(std::shared_ptr<ccl_comm> impl)
        : comm_impl(impl),
          comm_attr(ccl::create_device_comm_split_attr()),
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
    static ccl::communicator_interface::device_t empty;
    return empty;
}

ccl::communicator_interface::context_t host_communicator::get_context() {
    throw ccl::exception(std::string(__FUNCTION__) + " is not applicable for " + traits::name());
    static ccl::communicator_interface::context_t empty;
    return empty;
}

ccl::unique_ptr_class<host_communicator> host_communicator::split(const comm_split_attr& attr) {
    ccl::global_data& data = ccl::global_data::get();
    auto new_comm = ccl_comm::create_with_color(
        attr.get<ccl::comm_split_attr_id::color>(), data.comm_ids.get(), comm_impl.get());

    comm_attr = attr;

    return unique_ptr_class<host_communicator>(
        new host_communicator(std::shared_ptr<ccl_comm>(new_comm)));
}

host_communicator::coll_request_t host_communicator::barrier(
    const ccl::stream::impl_value_t& op_stream,
    const ccl::barrier_attr& attr,
    const ccl::vector_class<ccl::event>& deps) {
    return get_impl()->barrier_impl(op_stream, attr, deps);
}

host_communicator::coll_request_t host_communicator::barrier_impl(
    const ccl::stream::impl_value_t& op_stream,
    const ccl::barrier_attr& attr,
    const ccl::vector_class<ccl::event>& deps) {
    // TODO what exactly we need to do with 'attr' here?

    ccl_barrier_impl(comm_impl.get(), op_stream.get());

    // TODO what exactly we need to return here? ccl_barrier_impl() is void func
    ccl_request* req = nullptr;
    return std::unique_ptr<ccl::request_impl>(new ccl::host_request_impl(req));
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
    ccl_request* req = ccl_allgatherv_impl(
        send_buf, send_count, recv_buf, recv_counts.data(), dtype, attr, comm_impl.get(), nullptr);

    return std::unique_ptr<ccl::request_impl>(new ccl::host_request_impl(req));
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
    ccl_request* req =
        ccl_allreduce_impl(send_buf, recv_buf, count, dtype, reduction, attr, comm_impl.get(), nullptr);

    return std::unique_ptr<ccl::request_impl>(new ccl::host_request_impl(req));
}

/* alltoall */
host_communicator::coll_request_t host_communicator::alltoall_impl(
    const void* send_buf,
    void* recv_buf,
    size_t count,
    ccl::datatype dtype,
    const ccl::stream::impl_value_t& stream,
    const ccl::alltoall_attr& attr,
    const ccl::vector_class<ccl::event>& deps) {
    ccl_request* req =
        ccl_alltoall_impl(send_buf, recv_buf, count, dtype, attr, comm_impl.get(), nullptr);

    return std::unique_ptr<ccl::request_impl>(new ccl::host_request_impl(req));
}

host_communicator::coll_request_t host_communicator::alltoall_impl(
    const ccl::vector_class<void*>& send_buf,
    const ccl::vector_class<void*>& recv_buf,
    size_t count,
    ccl::datatype dtype,
    const ccl::stream::impl_value_t& stream,
    const ccl::alltoall_attr& attr,
    const ccl::vector_class<ccl::event>& deps) {
    // TODO not implemented
    throw ccl::exception(std::string(__PRETTY_FUNCTION__) + " - is not implemented");
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
    const ccl::vector_class<ccl::event>& deps) {
    ccl_request* req = ccl_alltoallv_impl(send_buf,
                                          send_counts.data(),
                                          recv_buf,
                                          recv_counts.data(),
                                          dtype,
                                          attr,
                                          comm_impl.get(),
                                          nullptr);

    return std::unique_ptr<ccl::request_impl>(new ccl::host_request_impl(req));
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
    // TODO not implemented
    throw ccl::exception(std::string(__PRETTY_FUNCTION__) + " - is not implemented");
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
    const ccl::vector_class<ccl::event>& deps) {
    ccl_request* req = ccl_broadcast_impl(buf, count, dtype, root, attr, comm_impl.get(), nullptr);

    return std::unique_ptr<ccl::request_impl>(new ccl::host_request_impl(req));
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
    const ccl::vector_class<ccl::event>& deps) {
    ccl_request* req = ccl_reduce_impl(
        send_buf, recv_buf, count, dtype, reduction, root, attr, comm_impl.get(), nullptr);

    return std::unique_ptr<ccl::request_impl>(new ccl::host_request_impl(req));
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
    const ccl::vector_class<ccl::event>& deps) {
    ccl_request* req = ccl_reduce_scatter_impl(
        send_buf, recv_buf, recv_count, dtype, reduction, attr, comm_impl.get(), nullptr);

    return std::unique_ptr<ccl::request_impl>(new ccl::host_request_impl(req));
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
    const ccl::vector_class<ccl::event>& deps) {
    ccl_request* req = ccl_sparse_allreduce_impl(send_ind_buf,
                                                 send_ind_count,
                                                 send_val_buf,
                                                 send_val_count,
                                                 recv_ind_buf,
                                                 recv_ind_count,
                                                 recv_val_buf,
                                                 recv_val_count,
                                                 index_dtype,
                                                 value_dtype,
                                                 reduction,
                                                 attr,
                                                 comm_impl.get(),
                                                 nullptr);

    return std::unique_ptr<ccl::request_impl>(new ccl::host_request_impl(req));
}

std::shared_ptr<atl_wrapper> host_communicator::get_atl() {
    return comm_impl->atl;
}

std::string host_communicator::to_string() const {
    return std::string("host communicator, rank (") + std::to_string(rank()) + "/" +
           std::to_string(size());
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
