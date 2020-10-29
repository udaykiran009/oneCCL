#pragma once

namespace ccl {

namespace v1 {

/******************** COMMUNICATOR ********************/

/**
 * Generating API types for collective operations
 * of the communicator class (communicator)
 */
#define API_COMM_OP_PTR_EXPLICIT_INSTANTIATION(BufferType) \
\
    template event CCL_API allgatherv(const BufferType* send_buf, \
                                        size_t send_count, \
                                        BufferType* recv_buf, \
                                        const vector_class<size_t>& recv_counts, \
                                        const communicator& comm, \
                                        const stream& op_stream, \
                                        const allgatherv_attr& attr, \
                                        const vector_class<event>& deps); \
\
    template event CCL_API allgatherv(const BufferType* send_buf, \
                                        size_t send_count, \
                                        BufferType* recv_buf, \
                                        const vector_class<size_t>& recv_counts, \
                                        const communicator& comm, \
                                        const allgatherv_attr& attr, \
                                        const vector_class<event>& deps); \
\
    template event CCL_API allgatherv(const BufferType* send_buf, \
                                        size_t send_count, \
                                        vector_class<BufferType*>& recv_bufs, \
                                        const vector_class<size_t>& recv_counts, \
                                        const communicator& comm, \
                                        const stream& op_stream, \
                                        const allgatherv_attr& attr, \
                                        const vector_class<event>& deps); \
\
    template event CCL_API allgatherv(const BufferType* send_buf, \
                                        size_t send_count, \
                                        vector_class<BufferType*>& recv_bufs, \
                                        const vector_class<size_t>& recv_counts, \
                                        const communicator& comm, \
                                        const allgatherv_attr& attr, \
                                        const vector_class<event>& deps); \
\
    template event CCL_API allreduce(const BufferType* send_buf, \
                                       BufferType* recv_buf, \
                                       size_t count, \
                                       reduction reduction, \
                                       const communicator& comm, \
                                       const stream& op_stream, \
                                       const allreduce_attr& attr, \
                                       const vector_class<event>& deps); \
\
    template event CCL_API allreduce(const BufferType* send_buf, \
                                       BufferType* recv_buf, \
                                       size_t count, \
                                       reduction reduction, \
                                       const communicator& comm, \
                                       const allreduce_attr& attr, \
                                       const vector_class<event>& deps); \
\
    template event CCL_API alltoall(const BufferType* send_buf, \
                                      BufferType* recv_buf, \
                                      size_t count, \
                                      const communicator& comm, \
                                      const stream& op_stream, \
                                      const alltoall_attr& attr, \
                                      const vector_class<event>& deps); \
\
    template event CCL_API alltoall(const BufferType* send_buf, \
                                      BufferType* recv_buf, \
                                      size_t count, \
                                      const communicator& comm, \
                                      const alltoall_attr& attr, \
                                      const vector_class<event>& deps); \
\
    template event CCL_API alltoall(const vector_class<BufferType*>& send_buf, \
                                      const vector_class<BufferType*>& recv_buf, \
                                      size_t count, \
                                      const communicator& comm, \
                                      const stream& op_stream, \
                                      const alltoall_attr& attr, \
                                      const vector_class<event>& deps); \
\
    template event CCL_API alltoall(const vector_class<BufferType*>& send_buf, \
                                      const vector_class<BufferType*>& recv_buf, \
                                      size_t count, \
                                      const communicator& comm, \
                                      const alltoall_attr& attr, \
                                      const vector_class<event>& deps); \
\
    template event CCL_API alltoallv(const BufferType* send_buf, \
                                       const vector_class<size_t>& send_counts, \
                                       BufferType* recv_buf, \
                                       const vector_class<size_t>& recv_counts, \
                                       const communicator& comm, \
                                       const stream& op_stream, \
                                       const alltoallv_attr& attr, \
                                       const vector_class<event>& deps); \
\
    template event CCL_API alltoallv(const BufferType* send_buf, \
                                       const vector_class<size_t>& send_counts, \
                                       BufferType* recv_buf, \
                                       const vector_class<size_t>& recv_counts, \
                                       const communicator& comm, \
                                       const alltoallv_attr& attr, \
                                       const vector_class<event>& deps); \
\
    template event CCL_API alltoallv(const vector_class<BufferType*>& send_bufs, \
                                       const vector_class<size_t>& send_counts, \
                                       const vector_class<BufferType*>& recv_bufs, \
                                       const vector_class<size_t>& recv_counts, \
                                       const communicator& comm, \
                                       const stream& op_stream, \
                                       const alltoallv_attr& attr, \
                                       const vector_class<event>& deps); \
\
    template event CCL_API alltoallv(const vector_class<BufferType*>& send_bufs, \
                                       const vector_class<size_t>& send_counts, \
                                       const vector_class<BufferType*>& recv_bufs, \
                                       const vector_class<size_t>& recv_counts, \
                                       const communicator& comm, \
                                       const alltoallv_attr& attr, \
                                       const vector_class<event>& deps); \
\
    template event CCL_API broadcast(BufferType* buf, \
                                       size_t count, \
                                       size_t root, \
                                       const communicator& comm, \
                                       const stream& op_stream, \
                                       const broadcast_attr& attr, \
                                       const vector_class<event>& deps); \
\
    template event CCL_API broadcast(BufferType* buf, \
                                       size_t count, \
                                       size_t root, \
                                       const communicator& comm, \
                                       const broadcast_attr& attr, \
                                       const vector_class<event>& deps); \
\
    template event CCL_API reduce(const BufferType* send_buf, \
                                    BufferType* recv_buf, \
                                    size_t count, \
                                    reduction reduction, \
                                    size_t root, \
                                    const communicator& comm, \
                                    const stream& op_stream, \
                                    const reduce_attr& attr, \
                                    const vector_class<event>& deps); \
\
    template event CCL_API reduce(const BufferType* send_buf, \
                                    BufferType* recv_buf, \
                                    size_t count, \
                                    reduction reduction, \
                                    size_t root, \
                                    const communicator& comm, \
                                    const reduce_attr& attr, \
                                    const vector_class<event>& deps); \
\
    template event CCL_API reduce_scatter(const BufferType* send_buf, \
                                            BufferType* recv_buf, \
                                            size_t recv_count, \
                                            reduction reduction, \
                                            const communicator& comm, \
                                            const stream& op_stream, \
                                            const reduce_scatter_attr& attr, \
                                            const vector_class<event>& deps); \
\
    template event CCL_API reduce_scatter(const BufferType* send_buf, \
                                            BufferType* recv_buf, \
                                            size_t recv_count, \
                                            reduction reduction, \
                                            const communicator& comm, \
                                            const reduce_scatter_attr& attr, \
                                            const vector_class<event>& deps);

#define API_COMM_OP_REF_EXPLICIT_INSTANTIATION(BufferObjectType) \
\
    template event CCL_API allgatherv(const BufferObjectType& send_buf, \
                                        size_t send_count, \
                                        BufferObjectType& recv_buf, \
                                        const vector_class<size_t>& recv_counts, \
                                        const communicator& comm, \
                                        const stream& op_stream, \
                                        const allgatherv_attr& attr, \
                                        const vector_class<event>& deps); \
\
    template event CCL_API allgatherv(const BufferObjectType& send_buf, \
                                        size_t send_count, \
                                        BufferObjectType& recv_buf, \
                                        const vector_class<size_t>& recv_counts, \
                                        const communicator& comm, \
                                        const allgatherv_attr& attr, \
                                        const vector_class<event>& deps); \
\
    template event CCL_API allgatherv( \
        const BufferObjectType& send_buf, \
        size_t send_count, \
        vector_class<reference_wrapper_class<BufferObjectType>>& recv_bufs, \
        const vector_class<size_t>& recv_counts, \
        const communicator& comm, \
        const stream& op_stream, \
        const allgatherv_attr& attr, \
        const vector_class<event>& deps); \
\
    template event CCL_API allgatherv( \
        const BufferObjectType& send_buf, \
        size_t send_count, \
        vector_class<reference_wrapper_class<BufferObjectType>>& recv_bufs, \
        const vector_class<size_t>& recv_counts, \
        const communicator& comm, \
        const allgatherv_attr& attr, \
        const vector_class<event>& deps); \
\
    template event CCL_API allreduce(const BufferObjectType& send_buf, \
                                       BufferObjectType& recv_buf, \
                                       size_t count, \
                                       reduction reduction, \
                                       const communicator& comm, \
                                       const stream& op_stream, \
                                       const allreduce_attr& attr, \
                                       const vector_class<event>& deps); \
\
    template event CCL_API allreduce(const BufferObjectType& send_buf, \
                                       BufferObjectType& recv_buf, \
                                       size_t count, \
                                       reduction reduction, \
                                       const communicator& comm, \
                                       const allreduce_attr& attr, \
                                       const vector_class<event>& deps); \
\
    template event CCL_API alltoall(const BufferObjectType& send_buf, \
                                      BufferObjectType& recv_buf, \
                                      size_t count, \
                                      const communicator& comm, \
                                      const stream& op_stream, \
                                      const alltoall_attr& attr, \
                                      const vector_class<event>& deps); \
\
    template event CCL_API alltoall(const BufferObjectType& send_buf, \
                                      BufferObjectType& recv_buf, \
                                      size_t count, \
                                      const communicator& comm, \
                                      const alltoall_attr& attr, \
                                      const vector_class<event>& deps); \
\
    template event CCL_API alltoall( \
        const vector_class<reference_wrapper_class<BufferObjectType>>& send_buf, \
        const vector_class<reference_wrapper_class<BufferObjectType>>& recv_buf, \
        size_t count, \
        const communicator& comm, \
        const stream& op_stream, \
        const alltoall_attr& attr, \
        const vector_class<event>& deps); \
\
    template event CCL_API alltoall( \
        const vector_class<reference_wrapper_class<BufferObjectType>>& send_buf, \
        const vector_class<reference_wrapper_class<BufferObjectType>>& recv_buf, \
        size_t count, \
        const communicator& comm, \
        const alltoall_attr& attr, \
        const vector_class<event>& deps); \
\
    template event CCL_API alltoallv(const BufferObjectType& send_buf, \
                                       const vector_class<size_t>& send_counts, \
                                       BufferObjectType& recv_buf, \
                                       const vector_class<size_t>& recv_counts, \
                                       const communicator& comm, \
                                       const stream& op_stream, \
                                       const alltoallv_attr& attr, \
                                       const vector_class<event>& deps); \
\
    template event CCL_API alltoallv(const BufferObjectType& send_buf, \
                                       const vector_class<size_t>& send_counts, \
                                       BufferObjectType& recv_buf, \
                                       const vector_class<size_t>& recv_counts, \
                                       const communicator& comm, \
                                       const alltoallv_attr& attr, \
                                       const vector_class<event>& deps); \
\
    template event CCL_API alltoallv( \
        const vector_class<reference_wrapper_class<BufferObjectType>>& send_bufs, \
        const vector_class<size_t>& send_counts, \
        const vector_class<reference_wrapper_class<BufferObjectType>>& recv_bufs, \
        const vector_class<size_t>& recv_counts, \
        const communicator& comm, \
        const stream& op_stream, \
        const alltoallv_attr& attr, \
        const vector_class<event>& deps); \
\
    template event CCL_API alltoallv( \
        const vector_class<reference_wrapper_class<BufferObjectType>>& send_bufs, \
        const vector_class<size_t>& send_counts, \
        const vector_class<reference_wrapper_class<BufferObjectType>>& recv_bufs, \
        const vector_class<size_t>& recv_counts, \
        const communicator& comm, \
        const alltoallv_attr& attr, \
        const vector_class<event>& deps); \
\
    template event CCL_API broadcast(BufferObjectType& buf, \
                                       size_t count, \
                                       size_t root, \
                                       const communicator& comm, \
                                       const stream& op_stream, \
                                       const broadcast_attr& attr, \
                                       const vector_class<event>& deps); \
\
    template event CCL_API broadcast(BufferObjectType& buf, \
                                       size_t count, \
                                       size_t root, \
                                       const communicator& comm, \
                                       const broadcast_attr& attr, \
                                       const vector_class<event>& deps); \
\
    template event CCL_API reduce(const BufferObjectType& send_buf, \
                                    BufferObjectType& recv_buf, \
                                    size_t count, \
                                    reduction reduction, \
                                    size_t root, \
                                    const communicator& comm, \
                                    const stream& op_stream, \
                                    const reduce_attr& attr, \
                                    const vector_class<event>& deps); \
\
    template event CCL_API reduce(const BufferObjectType& send_buf, \
                                    BufferObjectType& recv_buf, \
                                    size_t count, \
                                    reduction reduction, \
                                    size_t root, \
                                    const communicator& comm, \
                                    const reduce_attr& attr, \
                                    const vector_class<event>& deps); \
\
    template event CCL_API reduce_scatter(const BufferObjectType& send_buf, \
                                            BufferObjectType& recv_buf, \
                                            size_t recv_count, \
                                            reduction reduction, \
                                            const communicator& comm, \
                                            const stream& op_stream, \
                                            const reduce_scatter_attr& attr, \
                                            const vector_class<event>& deps); \
\
    template event CCL_API reduce_scatter(const BufferObjectType& send_buf, \
                                            BufferObjectType& recv_buf, \
                                            size_t recv_count, \
                                            reduction reduction, \
                                            const communicator& comm, \
                                            const reduce_scatter_attr& attr, \
                                            const vector_class<event>& deps);

} // namespace v1

namespace preview {

#define API_COMM_SPARSE_OP_PTR_EXPLICIT_INSTANTIATION(index_type, value_type) \
\
    template ccl::event CCL_API sparse_allreduce(const index_type* send_ind_buf, \
                                                   size_t send_ind_count, \
                                                   const value_type* send_val_buf, \
                                                   size_t send_val_count, \
                                                   index_type* recv_ind_buf, \
                                                   size_t recv_ind_count, \
                                                   value_type* recv_val_buf, \
                                                   size_t recv_val_count, \
                                                   ccl::reduction reduction, \
                                                   const ccl::communicator& comm, \
                                                   const ccl::stream& op_stream, \
                                                   const ccl::sparse_allreduce_attr& attr, \
                                                   const ccl::vector_class<ccl::event>& deps); \
\
    template ccl::event CCL_API sparse_allreduce(const index_type* send_ind_buf, \
                                                   size_t send_ind_count, \
                                                   const value_type* send_val_buf, \
                                                   size_t send_val_count, \
                                                   index_type* recv_ind_buf, \
                                                   size_t recv_ind_count, \
                                                   value_type* recv_val_buf, \
                                                   size_t recv_val_count, \
                                                   ccl::reduction reduction, \
                                                   const ccl::communicator& comm, \
                                                   const ccl::sparse_allreduce_attr& attr, \
                                                   const ccl::vector_class<ccl::event>& deps);

/*
#define API_COMM_SPARSE_OP_REF_EXPLICIT_INSTANTIATION(index_object_type, value_object_type) \
\
template ccl::event CCL_API \
sparse_allreduce(const index_object_type& send_ind_buf, \
                 size_t send_ind_count, \
                 const value_object_type& send_val_buf, \
                 size_t send_val_count, \
                 index_object_type& recv_ind_buf, \
                 size_t recv_ind_count, \
                 value_object_type& recv_val_buf, \
                 size_t recv_val_count, \
                 ccl::reduction reduction, \
                 const ccl::communicator& comm, \
                 const ccl::stream& op_stream, \
                 const ccl::sparse_allreduce_attr& attr, \
                 const ccl::vector_class<event>& deps); \
\
template ccl::event CCL_API \
sparse_allreduce(const index_object_type& send_ind_buf, \
                 size_t send_ind_count, \
                 const value_object_type& send_val_buf, \
                 size_t send_val_count, \
                 index_object_type& recv_ind_buf, \
                 size_t recv_ind_count, \
                 value_object_type& recv_val_buf, \
                 size_t recv_val_count, \
                 ccl::reduction reduction, \
                 const ccl::communicator& comm, \
                 const ccl::sparse_allreduce_attr& attr, \
                 const ccl::vector_class<event>& deps);
*/

} // namespace preview

} // namespace ccl
