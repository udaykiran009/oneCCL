#pragma once

/**
 * Generating types for collective operations
 * of the host implementation class (host_communicator_impl)
 */
#define HOST_COMM_IMPL_COLL_EXPLICIT_INSTANTIATIONS(comm_class, type) \
\
template ccl::communicator::request_t \
comm_class::allgatherv_impl(const type* send_buf, \
                            size_t send_count, \
                            type* recv_buf, \
                            const vector_class<size_t>& recv_counts, \
                            const allgatherv_attr_t& attr); \
\
template ccl::communicator::request_t \
comm_class::allgatherv_impl(const type* send_buf, \
                            size_t send_count, \
                            const vector_class<type*>& recv_bufs, \
                            const vector_class<size_t>& recv_counts, \
                            const allgatherv_attr_t& attr); \
\
template ccl::communicator::request_t \
comm_class::allreduce_impl(const type* send_buf, \
                           type* recv_buf, \
                           size_t count, \
                           ccl_reduction_t reduction, \
                           const allreduce_attr_t& attr); \
\
template ccl::communicator::request_t \
comm_class::alltoall_impl(const type* send_buf, \
                          type* recv_buf, \
                          size_t count, \
                          const alltoall_attr_t& attr); \
\
template ccl::communicator::request_t \
comm_class::alltoall_impl(const vector_class<type*>& send_buf, \
                          const vector_class<type*>& recv_buf, \
                          size_t count, \
                          const alltoall_attr_t& attr); \
\
template ccl::communicator::request_t \
comm_class::alltoallv_impl(const type* send_buf, \
                           const vector_class<size_t>& send_counts, \
                           type* recv_buf, \
                           const vector_class<size_t>& recv_counts, \
                           const alltoallv_attr_t& attr); \
\
template ccl::communicator::request_t \
comm_class::alltoallv_impl(const vector_class<type*>& send_bufs, \
                           const vector_class<size_t>& send_counts, \
                           const vector_class<type*>& recv_bufs, \
                           const vector_class<size_t>& recv_counts, \
                           const alltoallv_attr_t& attr); \
\
template ccl::communicator::request_t \
comm_class::bcast_impl(type* buf, \
                       size_t count, \
                       size_t root, \
                       const bcast_attr_t& attr); \
\
template ccl::communicator::request_t \
comm_class::reduce_impl(const type* send_buf, \
                        type* recv_buf, \
                        size_t count, \
                        ccl_reduction_t reduction, \
                        size_t root, \
                        const reduce_attr_t& attr); \
\
template ccl::communicator::request_t \
comm_class::reduce_scatter_impl(const type* send_buf, \
                                type* recv_buf, \
                                size_t recv_count, \
                                ccl_reduction_t reduction, \
                                const reduce_scatter_attr_t& attr); \


#define HOST_COMM_IMPL_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(comm_class, index_type, value_type) \
\
template ccl::communicator::request_t \
comm_class::sparse_allreduce_impl(const index_type* send_ind_buf, \
                                  size_t send_ind_count, \
                                  const value_type* send_val_buf, \
                                  size_t send_val_count, \
                                  index_type* recv_ind_buf, \
                                  size_t recv_ind_count, \
                                  value_type* recv_val_buf, \
                                  size_t recv_val_count, \
                                  ccl_reduction_t reduction, \
                                  const sparse_allreduce_attr_t& attr); \



/**
 * Generating API types for collective operations
 * of the host communicator class (communicator)
 */
#define API_HOST_COMM_COLL_EXPLICIT_INSTANTIATION(comm_class, type) \
\
template ccl::communicator::request_t CCL_API \
comm_class::allgatherv(const type* send_buf, \
                       size_t send_count, \
                       type* recv_buf, \
                       const vector_class<size_t>& recv_counts, \
                       const allgatherv_attr_t& attr); \
\
template ccl::communicator::request_t CCL_API \
comm_class::allgatherv(const type* send_buf, \
                       size_t send_count, \
                       const vector_class<type*>& recv_bufs, \
                       const vector_class<size_t>& recv_counts, \
                       const allgatherv_attr_t& attr); \
\
template ccl::communicator::request_t CCL_API \
comm_class::allreduce(const type* send_buf, \
                      type* recv_buf, \
                      size_t count, \
                      ccl_reduction_t reduction, \
                      const allreduce_attr_t& attr); \
\
template ccl::communicator::request_t CCL_API \
comm_class::alltoall(const type* send_buf, \
                     type* recv_buf, \
                     size_t count, \
                     const alltoall_attr_t& attr); \
\
template ccl::communicator::request_t CCL_API \
comm_class::alltoall(const vector_class<type*>& send_buf, \
                     const vector_class<type*>& recv_buf, \
                     size_t count, \
                     const alltoall_attr_t& attr); \
\
template ccl::communicator::request_t CCL_API \
comm_class::alltoallv(const type* send_buf, \
                      const vector_class<size_t>& send_counts, \
                      type* recv_buf, \
                      const vector_class<size_t>& recv_counts, \
                      const alltoallv_attr_t& attr); \
\
template ccl::communicator::request_t CCL_API \
comm_class::alltoallv(const vector_class<type*>& send_bufs, \
                      const vector_class<size_t>& send_counts, \
                      const vector_class<type*>& recv_bufs, \
                      const vector_class<size_t>& recv_counts, \
                      const alltoallv_attr_t& attr); \
\
template ccl::communicator::request_t CCL_API \
comm_class::bcast(type* buf, \
                  size_t count, \
                  size_t root, \
                  const bcast_attr_t& attr); \
\
template ccl::communicator::request_t CCL_API \
comm_class::reduce(const type* send_buf, \
                   type* recv_buf, \
                   size_t count, \
                   ccl_reduction_t reduction, \
                   size_t root, \
                   const reduce_attr_t& attr); \
\
template ccl::communicator::request_t CCL_API \
comm_class::reduce_scatter(const type* send_buf, \
                           type* recv_buf, \
                           size_t recv_count, \
                           ccl_reduction_t reduction, \
                           const reduce_scatter_attr_t& attr); \


#define API_HOST_COMM_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(comm_class, index_type, value_type) \
\
template ccl::communicator::request_t CCL_API \
comm_class::sparse_allreduce(const index_type* send_ind_buf, \
                             size_t send_ind_count, \
                             const value_type* send_val_buf, \
                             size_t send_val_count, \
                             index_type* recv_ind_buf, \
                             size_t recv_ind_count, \
                             value_type* recv_val_buf, \
                             size_t recv_val_count, \
                             ccl_reduction_t reduction, \
                             const sparse_allreduce_attr_t& attr); \

