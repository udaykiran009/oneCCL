#pragma once

namespace ccl {

#define CREATE_OP_ATTR_INSTANTIATION(attr) template attr CCL_API create_operation_attr<attr>();

/******************** HOST COMMUNICATOR ********************/

/**
 * Generating API types for collective operations
 * of the host communicator class (communicator)
 */
#define API_HOST_COMM_COLL_EXPLICIT_INSTANTIATION(type) \
\
    template request CCL_API allgatherv(const type* send_buf, \
                                        size_t send_count, \
                                        type* recv_buf, \
                                        const vector_class<size_t>& recv_counts, \
                                        const communicator& comm, \
                                        const allgatherv_attr& attr); \
\
    template request CCL_API allreduce(const type* send_buf, \
                                       type* recv_buf, \
                                       size_t count, \
                                       ccl::reduction reduction, \
                                       const communicator& comm, \
                                       const allreduce_attr& attr); \
\
    template request CCL_API alltoall(const type* send_buf, \
                                      type* recv_buf, \
                                      size_t count, \
                                      const communicator& comm, \
                                      const alltoall_attr& attr); \
\
    template request CCL_API alltoall(const vector_class<type*>& send_buf, \
                                      const vector_class<type*>& recv_buf, \
                                      size_t count, \
                                      const communicator& comm, \
                                      const alltoall_attr& attr); \
\
    template request CCL_API alltoallv(const type* send_buf, \
                                       const vector_class<size_t>& send_counts, \
                                       type* recv_buf, \
                                       const vector_class<size_t>& recv_counts, \
                                       const communicator& comm, \
                                       const alltoallv_attr& attr); \
\
    template request CCL_API alltoallv(const vector_class<type*>& send_bufs, \
                                       const vector_class<size_t>& send_counts, \
                                       const vector_class<type*>& recv_bufs, \
                                       const vector_class<size_t>& recv_counts, \
                                       const communicator& comm, \
                                       const alltoallv_attr& attr); \
\
    template request CCL_API broadcast( \
        type* buf, size_t count, size_t root, const communicator& comm, const broadcast_attr& attr); \
\
    template request CCL_API reduce(const type* send_buf, \
                                    type* recv_buf, \
                                    size_t count, \
                                    reduction reduction, \
                                    size_t root, \
                                    const communicator& comm, \
                                    const reduce_attr& attr); \
\
    template request CCL_API reduce_scatter(const type* send_buf, \
                                            type* recv_buf, \
                                            size_t recv_count, \
                                            reduction reduction, \
                                            const communicator& comm, \
                                            const reduce_scatter_attr& attr);

namespace preview {

#define API_HOST_COMM_SPARSE_ALLREDUCE_EXPLICIT_INSTANTIATION(index_type, value_type) \
\
    template ccl::request CCL_API sparse_allreduce(const index_type* send_ind_buf, \
                                                   size_t send_ind_count, \
                                                   const value_type* send_val_buf, \
                                                   size_t send_val_count, \
                                                   index_type* recv_ind_buf, \
                                                   size_t recv_ind_count, \
                                                   value_type* recv_val_buf, \
                                                   size_t recv_val_count, \
                                                   ccl::reduction reduction, \
                                                   const ccl::communicator& comm, \
                                                   const ccl::sparse_allreduce_attr& attr);

} // namespace preview

#if defined(MULTI_GPU_SUPPORT) || defined(CCL_ENABLE_SYCL)
/******************** DEVICE COMMUNICATOR ********************/

/**
 * Generating API types for collective operations
 * of the device communicator class (device_communicator)
 */
#define API_DEVICE_COMM_OP_PTR_EXPLICIT_INSTANTIATION(BufferType) \
\
    template request CCL_API allgatherv(const BufferType* send_buf, \
                                        size_t send_count, \
                                        BufferType* recv_buf, \
                                        const vector_class<size_t>& recv_counts, \
                                        const device_communicator& comm, \
                                        stream& op_stream, \
                                        const allgatherv_attr& attr, \
                                        const vector_class<event>& deps); \
\
    template request CCL_API allgatherv(const BufferType* send_buf, \
                                        size_t send_count, \
                                        vector_class<BufferType*>& recv_bufs, \
                                        const vector_class<size_t>& recv_counts, \
                                        const device_communicator& comm, \
                                        stream& op_stream, \
                                        const allgatherv_attr& attr, \
                                        const vector_class<event>& deps); \
\
    template request CCL_API allreduce(const BufferType* send_buf, \
                                       BufferType* recv_buf, \
                                       size_t count, \
                                       reduction reduction, \
                                       const device_communicator& comm, \
                                       stream& op_stream, \
                                       const allreduce_attr& attr, \
                                       const vector_class<event>& deps); \
\
    template request CCL_API alltoall(const BufferType* send_buf, \
                                      BufferType* recv_buf, \
                                      size_t count, \
                                      const device_communicator& comm, \
                                      stream& op_stream, \
                                      const alltoall_attr& attr, \
                                      const vector_class<event>& deps); \
\
    template request CCL_API alltoall(const vector_class<BufferType*>& send_buf, \
                                      const vector_class<BufferType*>& recv_buf, \
                                      size_t count, \
                                      const device_communicator& comm, \
                                      stream& op_stream, \
                                      const alltoall_attr& attr, \
                                      const vector_class<event>& deps); \
\
    template request CCL_API alltoallv(const BufferType* send_buf, \
                                       const vector_class<size_t>& send_counts, \
                                       BufferType* recv_buf, \
                                       const vector_class<size_t>& recv_counts, \
                                       const device_communicator& comm, \
                                       stream& op_stream, \
                                       const alltoallv_attr& attr, \
                                       const vector_class<event>& deps); \
\
    template request CCL_API alltoallv(const vector_class<BufferType*>& send_bufs, \
                                       const vector_class<size_t>& send_counts, \
                                       const vector_class<BufferType*>& recv_bufs, \
                                       const vector_class<size_t>& recv_counts, \
                                       const device_communicator& comm, \
                                       stream& op_stream, \
                                       const alltoallv_attr& attr, \
                                       const vector_class<event>& deps); \
\
    template request CCL_API broadcast(BufferType* buf, \
                                       size_t count, \
                                       size_t root, \
                                       const device_communicator& comm, \
                                       stream& op_stream, \
                                       const broadcast_attr& attr, \
                                       const vector_class<event>& deps); \
\
    template request CCL_API reduce(const BufferType* send_buf, \
                                    BufferType* recv_buf, \
                                    size_t count, \
                                    reduction reduction, \
                                    size_t root, \
                                    const device_communicator& comm, \
                                    stream& op_stream, \
                                    const reduce_attr& attr, \
                                    const vector_class<event>& deps); \
\
    template request CCL_API reduce_scatter(const BufferType* send_buf, \
                                            BufferType* recv_buf, \
                                            size_t recv_count, \
                                            reduction reduction, \
                                            const device_communicator& comm, \
                                            stream& op_stream, \
                                            const reduce_scatter_attr& attr, \
                                            const vector_class<event>& deps);

#define API_DEVICE_COMM_OP_REF_EXPLICIT_INSTANTIATION(BufferObjectType) \
\
    template request CCL_API allgatherv(const BufferObjectType& send_buf, \
                                        size_t send_count, \
                                        BufferObjectType& recv_buf, \
                                        const vector_class<size_t>& recv_counts, \
                                        const device_communicator& comm, \
                                        stream& op_stream, \
                                        const allgatherv_attr& attr, \
                                        const vector_class<event>& deps); \
\
    template request CCL_API allgatherv( \
        const BufferObjectType& send_buf, \
        size_t send_count, \
        vector_class<reference_wrapper_class<BufferObjectType>>& recv_bufs, \
        const vector_class<size_t>& recv_counts, \
        const device_communicator& comm, \
        stream& op_stream, \
        const allgatherv_attr& attr, \
        const vector_class<event>& deps); \
\
    template request CCL_API allreduce(const BufferObjectType& send_buf, \
                                       BufferObjectType& recv_buf, \
                                       size_t count, \
                                       reduction reduction, \
                                       const device_communicator& comm, \
                                       stream& op_stream, \
                                       const allreduce_attr& attr, \
                                       const vector_class<event>& deps); \
\
    template request CCL_API alltoall(const BufferObjectType& send_buf, \
                                      BufferObjectType& recv_buf, \
                                      size_t count, \
                                      const device_communicator& comm, \
                                      stream& op_stream, \
                                      const alltoall_attr& attr, \
                                      const vector_class<event>& deps); \
\
    template request CCL_API alltoall( \
        const vector_class<reference_wrapper_class<BufferObjectType>>& send_buf, \
        const vector_class<reference_wrapper_class<BufferObjectType>>& recv_buf, \
        size_t count, \
        const device_communicator& comm, \
        stream& op_stream, \
        const alltoall_attr& attr, \
        const vector_class<event>& deps); \
\
    template request CCL_API alltoallv(const BufferObjectType& send_buf, \
                                       const vector_class<size_t>& send_counts, \
                                       BufferObjectType& recv_buf, \
                                       const vector_class<size_t>& recv_counts, \
                                       const device_communicator& comm, \
                                       stream& op_stream, \
                                       const alltoallv_attr& attr, \
                                       const vector_class<event>& deps); \
\
    template request CCL_API alltoallv( \
        const vector_class<reference_wrapper_class<BufferObjectType>>& send_bufs, \
        const vector_class<size_t>& send_counts, \
        const vector_class<reference_wrapper_class<BufferObjectType>>& recv_bufs, \
        const vector_class<size_t>& recv_counts, \
        const device_communicator& comm, \
        stream& op_stream, \
        const alltoallv_attr& attr, \
        const vector_class<event>& deps); \
\
    template request CCL_API broadcast(BufferObjectType& buf, \
                                       size_t count, \
                                       size_t root, \
                                       const device_communicator& comm, \
                                       stream& op_stream, \
                                       const broadcast_attr& attr, \
                                       const vector_class<event>& deps); \
\
    template request CCL_API reduce(const BufferObjectType& send_buf, \
                                    BufferObjectType& recv_buf, \
                                    size_t count, \
                                    reduction reduction, \
                                    size_t root, \
                                    const device_communicator& comm, \
                                    stream& op_stream, \
                                    const reduce_attr& attr, \
                                    const vector_class<event>& deps); \
\
    template request CCL_API reduce_scatter(const BufferObjectType& send_buf, \
                                            BufferObjectType& recv_buf, \
                                            size_t recv_count, \
                                            reduction reduction, \
                                            const device_communicator& comm, \
                                            stream& op_stream, \
                                            const reduce_scatter_attr& attr, \
                                            const vector_class<event>& deps);

namespace preview {

#define API_DEVICE_COMM_SPARSE_OP_PTR_EXPLICIT_INSTANTIATION(index_type, value_type) \
\
    template ccl::request CCL_API sparse_allreduce(const index_type* send_ind_buf, \
                                                   size_t send_ind_count, \
                                                   const value_type* send_val_buf, \
                                                   size_t send_val_count, \
                                                   index_type* recv_ind_buf, \
                                                   size_t recv_ind_count, \
                                                   value_type* recv_val_buf, \
                                                   size_t recv_val_count, \
                                                   ccl::reduction reduction, \
                                                   const ccl::device_communicator& comm, \
                                                   ccl::stream& op_stream, \
                                                   const ccl::sparse_allreduce_attr& attr, \
                                                   const ccl::vector_class<ccl::event>& deps);

/*
#define API_DEVICE_COMM_SPARSE_OP_REF_EXPLICIT_INSTANTIATION(index_object_type, value_object_type) \
\
template ccl::request CCL_API \
sparse_allreduce(const index_object_type& send_ind_buf, \
                 size_t send_ind_count, \
                 const value_object_type& send_val_buf, \
                 size_t send_val_count, \
                 index_object_type& recv_ind_buf, \
                 size_t recv_ind_count, \
                 value_object_type& recv_val_buf, \
                 size_t recv_val_count, \
                 ccl::reduction reduction, \
                 const ccl::device_communicator& comm, \
                 ccl::stream& op_stream, \
                 const ccl::sparse_allreduce_attr& attr, \
                 const ccl::vector_class<event>& deps);
*/

} // namespace preview

#endif //#if defined(MULTI_GPU_SUPPORT) || defined(CCL_ENABLE_SYCL)

} // namespace ccl
