#include "ccl.hpp"
#include "ccl_type_traits.hpp"
#include "common/comm/single_device_communicator/single_device_communicator_impl.hpp"
#include "common/comm/l0/gpu_comm_attr.hpp"
#include "common/comm/l0/context/thread_group_ctx.hpp"
#include "common/comm/l0/context/process_group_ctx.hpp"

using namespace ccl;

single_device_communicator::single_device_communicator(ccl::unified_device_type&& device,
                                                               size_t thread_idx,
                                                               size_t process_idx,
                                                               const ccl::device_comm_split_attr_t& attr):
 base_t(std::move(device), thread_idx, process_idx/*, comm_attr*/, attr)
{
}

void single_device_communicator::set_ccl_comm(std::shared_ptr<ccl_comm> impl)
{
    CCL_ASSERT(!comm_impl, "comm_impl must be nullptr before first udage");
    comm_impl = impl;

    comm_rank = comm_impl->rank();
    comm_size = comm_impl->size();
}

void single_device_communicator::visit(ccl::gpu_comm_attr& comm_attr)
{
    auto process_ctx = comm_attr.get_process_context();
    auto thread_ctx = process_ctx->get_thread_context(process_id);
    auto device_ctx = thread_ctx->get_device_group_ctx(thread_id);

    //get rank & size

/*  this->initialize_comm_addr(get_device_path(),
                               ctx->get_group_topology<base_t::topology_class()>());
*/
    this->set_comm_group_id(comm_attr.get_unique_id());
}

ccl::request_t single_device_communicator::barrier(const ccl::barrier_attr_t& attr,
                 ccl::stream::impl_value_t& op_stream,
                 const ccl::vector_class<ccl::event>& deps)
{
    // TODO what exactly we need to do with 'attr' here?

    ccl_barrier_impl(comm_impl.get(), op_stream.get());

    // TODO what exactly we need to return here? ccl_barrier_impl() is void func
    ccl_request* req = nullptr;
    return std::unique_ptr<ccl::host_request_impl>(new ccl::host_request_impl(req));
}

/* allgatherv */
ccl::coll_request_t
single_device_communicator::allgatherv_impl(const void* send_buf,
                                                size_t send_count,
                                                void* recv_buf,
                                                const ccl::vector_class<size_t>& recv_counts,
                                                ccl_datatype_t dtype,
                                                const ccl::allgatherv_attr_t& attr,
                                                ccl::stream::impl_value_t& stream, const ccl::vector_class<ccl::event>& deps)
{
    ccl_request* req = ccl_allgatherv_impl(
            send_buf, send_count, recv_buf, recv_counts.data(),
            dtype, ccl_coll_attr(attr), comm_impl.get(), stream.get());

    return std::unique_ptr<ccl::host_request_impl>(new ccl::host_request_impl(req));
}
ccl::coll_request_t
single_device_communicator::allgatherv_impl(const void* send_buf,
                                             size_t send_count,
                                             const ccl::vector_class<void*>& recv_bufs,
                                            const ccl::vector_class<size_t>& recv_counts,
                                             ccl_datatype_t dtype,
                                             const ccl::allgatherv_attr_t& attr,
                                             ccl::stream::impl_value_t& stream,
                                             const ccl::vector_class<ccl::event>& deps)
{
    throw ccl::ccl_error(std::string(__PRETTY_FUNCTION__) + " - is not implemented");
    return {};
}


/* allreduce */
ccl::coll_request_t
single_device_communicator::allreduce_impl(const void* send_buf,
                                               void* recv_buf,
                                               size_t count,
                                               ccl_datatype_t dtype,
                                               ccl::reduction reduction,
                                               const ccl::allreduce_attr_t& attr,
                                               ccl::stream::impl_value_t& stream, const ccl::vector_class<ccl::event>& deps)
{
    ccl_request* req = ccl_allreduce_impl(send_buf, recv_buf, count, dtype,
                       static_cast<ccl_reduction_t>(reduction), ccl_coll_attr(attr), comm_impl.get(), stream.get());

    return std::unique_ptr<ccl::host_request_impl>(new ccl::host_request_impl(req));
}


/* alltoall */
ccl::coll_request_t
single_device_communicator::alltoall_impl(const void* send_buf,
                                             void* recv_buf,
                                             size_t count,
                                             ccl_datatype_t dtype,
                                             const ccl::alltoall_attr_t& attr,
                                             ccl::stream::impl_value_t& stream, const ccl::vector_class<ccl::event>& deps)
{
    ccl_request* req = ccl_alltoall_impl(send_buf, recv_buf, count, dtype,
                      ccl_coll_attr(attr), comm_impl.get(), stream.get());

    return std::unique_ptr<ccl::host_request_impl>(new ccl::host_request_impl(req));
}


ccl::coll_request_t
single_device_communicator::alltoall_impl(const ccl::vector_class<void*>& send_buf,
                       const ccl::vector_class<void*>& recv_buf,
                       size_t count,
                       ccl_datatype_t dtype,
                       const ccl::alltoall_attr_t& attr/* = alltoall_attr_t()*/,
                       ccl::stream::impl_value_t op_stream,
                       const ccl::vector_class<ccl::event>& deps)
{
    throw ccl::ccl_error(std::string(__PRETTY_FUNCTION__) + " - is not implemented");
    return {};
}

/* alltoallv */
ccl::coll_request_t
single_device_communicator::alltoallv_impl(const void* send_buf,
                                               const ccl::vector_class<size_t>& send_counts,
                                               void* recv_buf,
                                               const ccl::vector_class<size_t>& recv_counts,
                                               ccl_datatype_t dtype,
                                               const ccl::alltoallv_attr_t& attr,
                                               ccl::stream::impl_value_t& stream, const ccl::vector_class<ccl::event>& deps)
{
    ccl_request* req = ccl_alltoallv_impl(send_buf, send_counts.data(), recv_buf, recv_counts.data(),
                       dtype, ccl_coll_attr(attr), comm_impl.get(), stream.get());

    return std::unique_ptr<ccl::host_request_impl>(new ccl::host_request_impl(req));
}
ccl::coll_request_t
single_device_communicator::alltoallv_impl(const ccl::vector_class<void*>& send_buf,
                                                 const ccl::vector_class<size_t>& send_counts,
                                                 ccl::vector_class<void*> recv_buf,
                                                 const ccl::vector_class<size_t>& recv_counts,
                                                 ccl_datatype_t dtype,
                                                 const ccl::alltoallv_attr_t& attr,
                                                 ccl::stream::impl_value_t& stream,
                                                 const ccl::vector_class<ccl::event>& dep)
{
    throw ccl::ccl_error(std::string(__PRETTY_FUNCTION__) + " - is not implemented");
    return {};
}



/* bcast */
ccl::coll_request_t
single_device_communicator::bcast_impl(void* buf,
                                           size_t count,
                                           ccl_datatype_t dtype,
                                           size_t root,
                                           const ccl::bcast_attr_t& attr,
                                           ccl::stream::impl_value_t& stream, const ccl::vector_class<ccl::event>& deps)
{
    ccl_request* req = ccl_bcast_impl(buf, count, dtype, root,
                   ccl_coll_attr(attr), comm_impl.get(), stream.get());

    return std::unique_ptr<ccl::host_request_impl>(new ccl::host_request_impl(req));
}

/* reduce */
ccl::coll_request_t
single_device_communicator::reduce_impl(const void* send_buf,
                                            void* recv_buf,
                                            size_t count,
                                            ccl_datatype_t dtype,
                                            ccl::reduction reduction,
                                            size_t root,
                                            const ccl::reduce_attr_t& attr,
                                            ccl::stream::impl_value_t& stream, const ccl::vector_class<ccl::event>& deps)
{
    ccl_request* req = ccl_reduce_impl(send_buf, recv_buf, count, dtype,
                    static_cast<ccl_reduction_t>(reduction), root, ccl_coll_attr(attr), comm_impl.get(), stream.get());

    return std::unique_ptr<ccl::host_request_impl>(new ccl::host_request_impl(req));
}




/* reduce_scatter */
ccl::request_t
single_device_communicator::reduce_scatter_impl(const void* send_buf,
                             void* recv_buf,
                             size_t recv_count,
                             ccl_datatype_t dtype,
                             ccl::reduction reduction,
                             const ccl::reduce_scatter_attr_t& attr/* = reduce_scatter_attr_t()*/,
                             ccl::stream::impl_value_t& op_stream,
                             const ccl::vector_class<ccl::event>& deps)
{
   // TODO not implemented
    throw ccl_error(std::string(__PRETTY_FUNCTION__) + " - is not implemented");

    ccl_request* req = nullptr;
    return std::unique_ptr<ccl::host_request_impl>(new ccl::host_request_impl(req));
}



/* sparse_allreduce */
ccl::coll_request_t
single_device_communicator::sparse_allreduce_impl(const void* send_ind_buf, size_t send_ind_count,
                                                      const void* send_val_buf, size_t send_val_count,
                                                      void* recv_ind_buf, size_t recv_ind_count,
                                                      void* recv_val_buf, size_t recv_val_count,
                                                      ccl_datatype_t index_dtype,
                                                      ccl_datatype_t value_dtype,
                                                      ccl::reduction reduction,
                                                      const ccl::sparse_allreduce_attr_t& attr,
                                                      ccl::stream::impl_value_t& stream, const ccl::vector_class<ccl::event>& deps)
{
    ccl_request* req = ccl_sparse_allreduce_impl(send_ind_buf, send_ind_count,
                              send_val_buf, send_val_count,
                              recv_ind_buf, recv_ind_count,
                              recv_val_buf, recv_val_count,
                              index_dtype, value_dtype,
                              static_cast<ccl_reduction_t>(reduction), ccl_coll_attr(attr),
                              comm_impl.get(), stream.get());

    return std::unique_ptr<ccl::host_request_impl>(new ccl::host_request_impl(req));
}

DEVICE_COMM_INTERFACE_COLL_INSTANTIATIONS(single_device_communicator, char);
DEVICE_COMM_INTERFACE_COLL_INSTANTIATIONS(single_device_communicator, int);
DEVICE_COMM_INTERFACE_COLL_INSTANTIATIONS(single_device_communicator, int64_t);
DEVICE_COMM_INTERFACE_COLL_INSTANTIATIONS(single_device_communicator, uint64_t);
DEVICE_COMM_INTERFACE_COLL_INSTANTIATIONS(single_device_communicator, float);
DEVICE_COMM_INTERFACE_COLL_INSTANTIATIONS(single_device_communicator, double);

#ifdef CCL_ENABLE_SYCL
    DEVICE_COMM_INTERFACE_COLL_CLASS_INSTANTIATIONS(single_device_communicator, cl::sycl::buffer<char COMMA 1>);
    DEVICE_COMM_INTERFACE_COLL_CLASS_INSTANTIATIONS(single_device_communicator, cl::sycl::buffer<int COMMA 1>);
    DEVICE_COMM_INTERFACE_COLL_CLASS_INSTANTIATIONS(single_device_communicator, cl::sycl::buffer<int64_t COMMA 1>);
    DEVICE_COMM_INTERFACE_COLL_CLASS_INSTANTIATIONS(single_device_communicator, cl::sycl::buffer<uint64_t COMMA 1>);
    DEVICE_COMM_INTERFACE_COLL_CLASS_INSTANTIATIONS(single_device_communicator, cl::sycl::buffer<float COMMA 1>);
    DEVICE_COMM_INTERFACE_COLL_CLASS_INSTANTIATIONS(single_device_communicator, cl::sycl::buffer<double COMMA 1>);
#endif //CCL_ENABLE_SYCL

DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(single_device_communicator, char, char);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(single_device_communicator, char, int);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(single_device_communicator, char, ccl::bfp16);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(single_device_communicator, char, float);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(single_device_communicator, char, double);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(single_device_communicator, char, int64_t);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(single_device_communicator, char, uint64_t);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(single_device_communicator, int, char);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(single_device_communicator, int, int);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(single_device_communicator, int, ccl::bfp16);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(single_device_communicator, int, float);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(single_device_communicator, int, double);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(single_device_communicator, int, int64_t);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(single_device_communicator, int, uint64_t);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(single_device_communicator, int64_t, char);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(single_device_communicator, int64_t, int);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(single_device_communicator, int64_t, ccl::bfp16);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(single_device_communicator, int64_t, float);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(single_device_communicator, int64_t, double);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(single_device_communicator, int64_t, int64_t);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(single_device_communicator, int64_t, uint64_t);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(single_device_communicator, uint64_t, char);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(single_device_communicator, uint64_t, int);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(single_device_communicator, uint64_t, ccl::bfp16);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(single_device_communicator, uint64_t, float);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(single_device_communicator, uint64_t, double);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(single_device_communicator, uint64_t, int64_t);
DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(single_device_communicator, uint64_t, uint64_t);

#ifdef CCL_ENABLE_SYCL
    DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_CLASS_INSTANTIATION(single_device_communicator,
                                                                 cl::sycl::buffer<int COMMA 1>,
                                                                 cl::sycl::buffer<float COMMA 1>);
    DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_CLASS_INSTANTIATION(single_device_communicator,
                                                                 cl::sycl::buffer<int COMMA 1>,
                                                                 cl::sycl::buffer<ccl::bfp16 COMMA 1>);

    DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_CLASS_INSTANTIATION(single_device_communicator,
                                                                 cl::sycl::buffer<int64_t COMMA 1>,
                                                                 cl::sycl::buffer<float COMMA 1>);
    DEVICE_COMM_INTERFACE_SPARSE_ALLREDUCE_EXPLICIT_CLASS_INSTANTIATION(single_device_communicator,
                                                                 cl::sycl::buffer<int64_t COMMA 1>,
                                                                 cl::sycl::buffer<ccl::bfp16 COMMA 1>);
#endif //CCL_ENABLE_SYCL
