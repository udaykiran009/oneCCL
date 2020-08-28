#include "common/comm/host_communicator/host_communicator_impl.hpp"

namespace ccl {

host_communicator::host_communicator() :
    comm_rank(0), comm_size(1)
{
}

host_communicator::host_communicator(size_t size,
                                     shared_ptr_class<kvs_interface> kvs) :
    comm_rank(0), comm_size(size)
{
    if (size <= 0) {
        throw ccl_error("Incorrect size value when creating a host communicator");
    }
}

host_communicator::host_communicator(size_t size,
                                     size_t rank,
                                     shared_ptr_class<kvs_interface> kvs) :
    comm_rank(rank), comm_size(size)
{
    if (rank > size || size <= 0) {
        throw ccl_error("Incorrect rank or size value when creating a host communicator");
    }
}

size_t host_communicator::rank() const
{
    return comm_rank;
}

size_t host_communicator::size() const
{
    return comm_size;
}

/* allgatherv */
ccl::request_t
host_communicator::allgatherv_impl(const void* send_buf,
                                   size_t send_count,
                                   void* recv_buf,
                                   const vector_class<size_t>& recv_counts,
                                   ccl_datatype_t dtype,
                                   const allgatherv_attr_t& attr)
{
    // TODO There must be a call to a collective function
    return ccl::request_t();
}

ccl::request_t
host_communicator::allgatherv_impl(const void* send_buf,
                                   size_t send_count,
                                   const vector_class<void*>& recv_bufs,
                                   const vector_class<size_t>& recv_counts,
                                   ccl_datatype_t dtype,
                                   const allgatherv_attr_t& attr)
{
    // TODO There must be a call to a collective function
    return ccl::request_t();
}

/* allreduce */
ccl::request_t
host_communicator::allreduce_impl(const void* send_buf,
                                  void* recv_buf,
                                  size_t count,
                                  ccl_datatype_t dtype,
                                  ccl_reduction_t reduction,
                                  const allreduce_attr_t& attr)
{
    // TODO There must be a call to a collective function
    return ccl::request_t();
}

/* alltoall */
ccl::request_t
host_communicator::alltoall_impl(const void* send_buf,
                                 void* recv_buf,
                                 size_t count,
                                 ccl_datatype_t dtype,
                                 const alltoall_attr_t& attr)
{
    // TODO There must be a call to a collective function
    return ccl::request_t();
}

ccl::request_t
host_communicator::alltoall_impl(const vector_class<void*>& send_buf,
                                 const vector_class<void*>& recv_buf,
                                 size_t count,
                                 ccl_datatype_t dtype,
                                 const alltoall_attr_t& attr)
{
    // TODO There must be a call to a collective function
    return ccl::request_t();
}

/* alltoallv */
ccl::request_t
host_communicator::alltoallv_impl(const void* send_buf,
                                  const vector_class<size_t>& send_counts,
                                  void* recv_buf,
                                  const vector_class<size_t>& recv_counts,
                                  ccl_datatype_t dtype,
                                  const alltoallv_attr_t& attr)
{
    // TODO There must be a call to a collective function
    return ccl::request_t();
}

ccl::request_t
host_communicator::alltoallv_impl(const vector_class<void*>& send_bufs,
                                  const vector_class<size_t>& send_counts,
                                  const vector_class<void*>& recv_bufs,
                                  const vector_class<size_t>& recv_counts,
                                  ccl_datatype_t dtype,
                                  const alltoallv_attr_t& attr)
{
    // TODO There must be a call to a collective function
    return ccl::request_t();
}

/* barrier */
ccl::request_t
host_communicator::barrier_impl(const barrier_attr_t& attr)
{
    // TODO There must be a call to a collective function
    return ccl::request_t();
}

/* bcast */
ccl::request_t
host_communicator::bcast_impl(void* buf,
                              size_t count,
                              ccl_datatype_t dtype,
                              size_t root,
                              const bcast_attr_t& attr)
{
    // TODO There must be a call to a collective function
    return ccl::request_t();
}

/* reduce */
ccl::request_t
host_communicator::reduce_impl(const void* send_buf,
                               void* recv_buf,
                               size_t count,
                               ccl_datatype_t dtype,
                               ccl_reduction_t reduction,
                               size_t root,
                               const reduce_attr_t& attr)
{
    // TODO There must be a call to a collective function
    return ccl::request_t();
}

/* reduce_scatter */
ccl::request_t
host_communicator::reduce_scatter_impl(const void* send_buf,
                                       void* recv_buf,
                                       size_t recv_count,
                                       ccl_datatype_t dtype,
                                       ccl_reduction_t reduction,
                                       const reduce_scatter_attr_t& attr)
{
    // TODO There must be a call to a collective function
    return ccl::request_t();
}

/* sparse_allreduce */
ccl::request_t
host_communicator::sparse_allreduce_impl(const void* send_ind_buf,
                                         size_t send_ind_count,
                                         const void* send_val_buf,
                                         size_t send_val_count,
                                         void* recv_ind_buf,
                                         size_t recv_ind_count,
                                         void* recv_val_buf,
                                         size_t recv_val_count,
                                         ccl_datatype_t ind_dtype,
                                         ccl_datatype_t val_dtype,
                                         ccl_reduction_t reduction,
                                         const sparse_allreduce_attr_t& attr)
{
    // TODO There must be a call to a collective function
    return ccl::request_t();
}

HOST_COMM_IMPL_COLL_EXPLICIT_INSTANTIATIONS(host_communicator, char);
HOST_COMM_IMPL_COLL_EXPLICIT_INSTANTIATIONS(host_communicator, int);
HOST_COMM_IMPL_COLL_EXPLICIT_INSTANTIATIONS(host_communicator, int64_t);
HOST_COMM_IMPL_COLL_EXPLICIT_INSTANTIATIONS(host_communicator, uint64_t);
HOST_COMM_IMPL_COLL_EXPLICIT_INSTANTIATIONS(host_communicator, float);
HOST_COMM_IMPL_COLL_EXPLICIT_INSTANTIATIONS(host_communicator, double);

HOST_COMM_IMPL_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(host_communicator, char, char);
HOST_COMM_IMPL_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(host_communicator, char, int);
HOST_COMM_IMPL_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(host_communicator, char, ccl::bfp16);
HOST_COMM_IMPL_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(host_communicator, char, float);
HOST_COMM_IMPL_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(host_communicator, char, double);
HOST_COMM_IMPL_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(host_communicator, char, int64_t);
HOST_COMM_IMPL_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(host_communicator, char, uint64_t);
HOST_COMM_IMPL_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(host_communicator, int, char);
HOST_COMM_IMPL_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(host_communicator, int, int);
HOST_COMM_IMPL_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(host_communicator, int, ccl::bfp16);
HOST_COMM_IMPL_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(host_communicator, int, float);
HOST_COMM_IMPL_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(host_communicator, int, double);
HOST_COMM_IMPL_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(host_communicator, int, int64_t);
HOST_COMM_IMPL_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(host_communicator, int, uint64_t);
HOST_COMM_IMPL_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(host_communicator, int64_t, char);
HOST_COMM_IMPL_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(host_communicator, int64_t, int);
HOST_COMM_IMPL_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(host_communicator, int64_t, ccl::bfp16);
HOST_COMM_IMPL_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(host_communicator, int64_t, float);
HOST_COMM_IMPL_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(host_communicator, int64_t, double);
HOST_COMM_IMPL_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(host_communicator, int64_t, int64_t);
HOST_COMM_IMPL_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(host_communicator, int64_t, uint64_t);
HOST_COMM_IMPL_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(host_communicator, uint64_t, char);
HOST_COMM_IMPL_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(host_communicator, uint64_t, int);
HOST_COMM_IMPL_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(host_communicator, uint64_t, ccl::bfp16);
HOST_COMM_IMPL_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(host_communicator, uint64_t, float);
HOST_COMM_IMPL_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(host_communicator, uint64_t, double);
HOST_COMM_IMPL_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(host_communicator, uint64_t, int64_t);
HOST_COMM_IMPL_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(host_communicator, uint64_t, uint64_t);

} // namespace ccl
