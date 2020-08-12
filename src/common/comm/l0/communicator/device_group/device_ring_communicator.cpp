#include "ccl.hpp"
#include "ccl_type_traits.hpp"
#include "common/comm/l0/communicator/device_group/device_ring_communicator_impl.hpp"
#include "common/comm/l0/gpu_comm_attr.hpp"
#include "common/comm/l0/context/thread_group_ctx.hpp"
#include "common/comm/l0/context/process_group_ctx.hpp"

using namespace ccl;

device_group_ring_communicator::device_group_ring_communicator(ccl::unified_device_type&& device,
                                                               size_t thread_idx,
                                                               size_t process_idx,
                                                               const ccl::device_comm_split_attr_t& attr):
 base_t(std::move(device), thread_idx, process_idx/*, comm_attr*/, attr)
{
}

void device_group_ring_communicator::visit(ccl::gpu_comm_attr& comm_attr)
{
    auto process_ctx = comm_attr.get_process_context();
    auto thread_ctx = process_ctx->get_thread_context(process_id);
    auto device_ctx = thread_ctx->get_device_group_ctx(thread_id);

    ctx = device_ctx;

    //get rank & size

    this->initialize_comm_addr(get_device_path(),
                               ctx->get_group_topology<base_t::topology_class()>());
}

void device_group_ring_communicator::barrier(const barrier_attr_t& attr,
                 ccl::stream::impl_value_t& op_stream,
                 const vector_class<event>& deps)
{
    throw ccl::ccl_error(std::string(__PRETTY_FUNCTION__) + " - is not implemented yet");
}

/* allgatherv */
ccl::communicator::coll_request_t
device_group_ring_communicator::allgatherv_impl(const void* send_buf,
                                                size_t send_count,
                                                void* recv_buf,
                                                const size_t* recv_counts,
                                                ccl_datatype_t dtype,
                                                const ccl::coll_attr* attr,
                                                ccl::stream::impl_value_t& stream)
{
    throw ccl::ccl_error(std::string(__PRETTY_FUNCTION__) + " - is not implemented");
    return {};
}

/* allreduce */
ccl::communicator::coll_request_t
device_group_ring_communicator::allreduce_impl(const void* send_buf,
                                               void* recv_buf,
                                               size_t count,
                                               ccl_datatype_t dtype,
                                               ccl::reduction reduction,
                                               const ccl::coll_attr* attr,
                                               ccl::stream::impl_value_t& stream)
{
    throw ccl::ccl_error(std::string(__PRETTY_FUNCTION__) + " - is not implemented");
    return {};
}


/* alltoall */
ccl::communicator::coll_request_t
device_group_ring_communicator::alltoall_impl(const void* send_buf,
                                             void* recv_buf,
                                             size_t count,
                                             ccl_datatype_t dtype,
                                             const ccl::coll_attr* attr,
                                             ccl::stream::impl_value_t& stream)
{
    throw ccl::ccl_error(std::string(__PRETTY_FUNCTION__) + " - is not implemented");
    return {};
}


/* alltoallv */
ccl::communicator::coll_request_t
device_group_ring_communicator::alltoallv_impl(const void* send_buf,
                                               const size_t* send_counts,
                                               void* recv_buf,
                                               const size_t* recv_counts,
                                               ccl_datatype_t dtype,
                                               const ccl::coll_attr* attr,
                                               ccl::stream::impl_value_t& stream)
{
    throw ccl::ccl_error(std::string(__PRETTY_FUNCTION__) + " - is not implemented");
    return {};
}


/* bcast */
ccl::communicator::coll_request_t
device_group_ring_communicator::bcast_impl(void* buf,
                                           size_t count,
                                           ccl_datatype_t dtype,
                                           size_t root,
                                           const ccl::coll_attr* attr,
                                           ccl::stream::impl_value_t& stream)
{
    throw ccl::ccl_error(std::string(__PRETTY_FUNCTION__) + " - is not implemented");
    return {};
}

/* reduce */
ccl::communicator::coll_request_t
device_group_ring_communicator::reduce_impl(const void* send_buf,
                                            void* recv_buf,
                                            size_t count,
                                            ccl_datatype_t dtype,
                                            ccl::reduction reduction,
                                            size_t root,
                                            const ccl::coll_attr* attr,
                                            ccl::stream::impl_value_t& stream)
{
    throw ccl::ccl_error(std::string(__PRETTY_FUNCTION__) + " - is not implemented");
    return {};
}


/* sparse_allreduce */
ccl::communicator::coll_request_t
device_group_ring_communicator::sparse_allreduce_impl(const void* send_ind_buf, size_t send_ind_count,
                                                      const void* send_val_buf, size_t send_val_count,
                                                      void* recv_ind_buf, size_t recv_ind_count,
                                                      void* recv_val_buf, size_t recv_val_count,
                                                      ccl_datatype_t index_dtype,
                                                      ccl_datatype_t value_dtype,
                                                      ccl::reduction reduction,
                                                      const ccl::coll_attr* attr,
                                                      ccl::stream::impl_value_t& stream)
{
    throw ccl::ccl_error(std::string(__PRETTY_FUNCTION__) + " - is not implemented");
    return {};
}

DEVICE_COMM_INTERFACE_COLL_INSTANTIATIONS(device_group_ring_communicator, char);
DEVICE_COMM_INTERFACE_COLL_INSTANTIATIONS(device_group_ring_communicator, int);
DEVICE_COMM_INTERFACE_COLL_INSTANTIATIONS(device_group_ring_communicator, int64_t);
DEVICE_COMM_INTERFACE_COLL_INSTANTIATIONS(device_group_ring_communicator, uint64_t);
DEVICE_COMM_INTERFACE_COLL_INSTANTIATIONS(device_group_ring_communicator, float);
DEVICE_COMM_INTERFACE_COLL_INSTANTIATIONS(device_group_ring_communicator, double);

#ifdef CCL_ENABLE_SYCL
    DEVICE_COMM_INTERFACE_COLL_CLASS_INSTANTIATIONS(device_group_ring_communicator, cl::sycl::buffer<char COMMA 1>);
    DEVICE_COMM_INTERFACE_COLL_CLASS_INSTANTIATIONS(device_group_ring_communicator, cl::sycl::buffer<int COMMA 1>);
    DEVICE_COMM_INTERFACE_COLL_CLASS_INSTANTIATIONS(device_group_ring_communicator, cl::sycl::buffer<int64_t COMMA 1>);
    DEVICE_COMM_INTERFACE_COLL_CLASS_INSTANTIATIONS(device_group_ring_communicator, cl::sycl::buffer<uint64_t COMMA 1>);
    DEVICE_COMM_INTERFACE_COLL_CLASS_INSTANTIATIONS(device_group_ring_communicator, cl::sycl::buffer<float COMMA 1>);
    DEVICE_COMM_INTERFACE_COLL_CLASS_INSTANTIATIONS(device_group_ring_communicator, cl::sycl::buffer<double COMMA 1>);
#endif //CCL_ENABLE_SYCL

DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(device_group_ring_communicator, char, char);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(device_group_ring_communicator, char, int);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(device_group_ring_communicator, char, ccl::bfp16);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(device_group_ring_communicator, char, float);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(device_group_ring_communicator, char, double);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(device_group_ring_communicator, char, int64_t);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(device_group_ring_communicator, char, uint64_t);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(device_group_ring_communicator, int, char);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(device_group_ring_communicator, int, int);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(device_group_ring_communicator, int, ccl::bfp16);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(device_group_ring_communicator, int, float);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(device_group_ring_communicator, int, double);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(device_group_ring_communicator, int, int64_t);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(device_group_ring_communicator, int, uint64_t);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(device_group_ring_communicator, int64_t, char);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(device_group_ring_communicator, int64_t, int);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(device_group_ring_communicator, int64_t, ccl::bfp16);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(device_group_ring_communicator, int64_t, float);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(device_group_ring_communicator, int64_t, double);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(device_group_ring_communicator, int64_t, int64_t);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(device_group_ring_communicator, int64_t, uint64_t);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(device_group_ring_communicator, uint64_t, char);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(device_group_ring_communicator, uint64_t, int);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(device_group_ring_communicator, uint64_t, ccl::bfp16);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(device_group_ring_communicator, uint64_t, float);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(device_group_ring_communicator, uint64_t, double);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(device_group_ring_communicator, uint64_t, int64_t);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(device_group_ring_communicator, uint64_t, uint64_t);

#ifdef CCL_ENABLE_SYCL
    DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_CLASS_INSTANTIATION(device_group_ring_communicator,
                                                                 cl::sycl::buffer<int COMMA 1>,
                                                                 cl::sycl::buffer<float COMMA 1>);
    DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_CLASS_INSTANTIATION(device_group_ring_communicator,
                                                                 cl::sycl::buffer<int COMMA 1>,
                                                                 cl::sycl::buffer<ccl::bfp16 COMMA 1>);

    DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_CLASS_INSTANTIATION(device_group_ring_communicator,
                                                                 cl::sycl::buffer<int64_t COMMA 1>,
                                                                 cl::sycl::buffer<float COMMA 1>);
    DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_CLASS_INSTANTIATION(device_group_ring_communicator,
                                                                 cl::sycl::buffer<int64_t COMMA 1>,
                                                                 cl::sycl::buffer<ccl::bfp16 COMMA 1>);
#endif //CCL_ENABLE_SYCL
