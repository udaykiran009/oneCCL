#pragma once

namespace ccl {

class host_communicator
{
public:
    size_t rank() const;
    size_t size() const;

    /* allgatherv */
    ccl::communicator::request_t
    allgatherv_impl(const void* send_buf,
                    size_t send_count,
                    void* recv_buf,
                    const vector_class<size_t>& recv_counts,
                    ccl_datatype_t dtype,
                    const allgatherv_attr_t& attr);

    ccl::communicator::request_t
    allgatherv_impl(const void* send_buf,
                    size_t send_count,
                    const vector_class<void*>& recv_bufs,
                    const vector_class<size_t>& recv_counts,
                    ccl_datatype_t dtype,
                    const allgatherv_attr_t& attr);

    template<class BufferType>
    ccl::communicator::request_t
    allgatherv_impl(const BufferType* send_buf,
                    size_t send_count,
                    BufferType* recv_buf,
                    const vector_class<size_t>& recv_counts,
                    const allgatherv_attr_t& attr);

    template<class BufferType>
    ccl::communicator::request_t
    allgatherv_impl(const BufferType* send_buf,
                    size_t send_count,
                    const vector_class<BufferType*>& recv_bufs,
                    const vector_class<size_t>& recv_counts,
                    const allgatherv_attr_t& attr);

    /* allreduce */
    ccl::communicator::request_t
    allreduce_impl(const void* send_buf,
                   void* recv_buf,
                   size_t count,
                   ccl_datatype_t dtype,
                   ccl_reduction_t reduction,
                   const allreduce_attr_t& attr);

    template <class BufferType,
              class = typename std::enable_if<ccl::is_native_type_supported<BufferType>()>::type>
    ccl::communicator::request_t
    allreduce_impl(const BufferType* send_buf,
                   BufferType* recv_buf,
                   size_t count,
                   ccl_reduction_t reduction,
                   const allreduce_attr_t& attr);

    /* alltoall */
    ccl::communicator::request_t
    alltoall_impl(const void* send_buf,
                  void* recv_buf,
                  size_t count,
                  ccl_datatype_t dtype,
                  const alltoall_attr_t& attr);

    ccl::communicator::request_t
    alltoall_impl(const vector_class<void*>& send_buf,
                  const vector_class<void*>& recv_buf,
                  size_t count,
                  ccl_datatype_t dtype,
                  const alltoall_attr_t& attr);

    template <class BufferType,
              class = typename std::enable_if<ccl::is_native_type_supported<BufferType>()>::type>
    ccl::communicator::request_t
    alltoall_impl(const BufferType* send_buf,
                  BufferType* recv_buf,
                  size_t count,
                  const alltoall_attr_t& attr);

    template <class BufferType,
              class = typename std::enable_if<ccl::is_native_type_supported<BufferType>()>::type>
    ccl::communicator::request_t
    alltoall_impl(const vector_class<BufferType*>& send_buf,
                  const vector_class<BufferType*>& recv_buf,
                  size_t count,
                  const alltoall_attr_t& attr);

    /* alltoallv */
    ccl::communicator::request_t
    alltoallv_impl(const void* send_buf,
                   const vector_class<size_t>& send_counts,
                   void* recv_buf,
                   const vector_class<size_t>& recv_counts,
                   ccl_datatype_t dtype,
                   const alltoallv_attr_t& attr);

    ccl::communicator::request_t
    alltoallv_impl(const vector_class<void*>& send_bufs,
                   const vector_class<size_t>& send_counts,
                   const vector_class<void*>& recv_bufs,
                   const vector_class<size_t>& recv_counts,
                   ccl_datatype_t dtype,
                   const alltoallv_attr_t& attr);

    template <class BufferType,
              class = typename std::enable_if<ccl::is_native_type_supported<BufferType>()>::type>
    ccl::communicator::request_t
    alltoallv_impl(const BufferType* send_buf,
                   const vector_class<size_t>& send_counts,
                   BufferType* recv_buf,
                   const vector_class<size_t>& recv_counts,
                   const alltoallv_attr_t& attr);

    template <class BufferType,
              class = typename std::enable_if<ccl::is_native_type_supported<BufferType>()>::type>
    ccl::communicator::request_t
    alltoallv_impl(const vector_class<BufferType*>& send_bufs,
                   const vector_class<size_t>& send_counts,
                   const vector_class<BufferType*>& recv_bufs,
                   const vector_class<size_t>& recv_counts,
                   const alltoallv_attr_t& attr);

    /* barrier */
    communicator::request_t
    barrier_impl(const barrier_attr_t& attr);

    /* bcast */
    ccl::communicator::request_t
    bcast_impl(void* buf,
               size_t count,
               ccl_datatype_t dtype,
               size_t root,
               const bcast_attr_t& attr);

    template <class BufferType,
              class = typename std::enable_if<ccl::is_native_type_supported<BufferType>()>::type>
    ccl::communicator::request_t
    bcast_impl(BufferType* buf,
               size_t count,
               size_t root,
               const bcast_attr_t& attr);

    /* reduce */
    ccl::communicator::request_t
    reduce_impl(const void* send_buf,
                void* recv_buf,
                size_t count,
                ccl_datatype_t dtype,
                ccl_reduction_t reduction,
                size_t root,
                const reduce_attr_t& attr);

    template <class BufferType,
              class = typename std::enable_if<ccl::is_native_type_supported<BufferType>()>::type>
    ccl::communicator::request_t
    reduce_impl(const BufferType* send_buf,
                BufferType* recv_buf,
                size_t count,
                ccl_reduction_t reduction,
                size_t root,
                const reduce_attr_t& attr);

    /* reduce_scatter */
    ccl::communicator::request_t
    reduce_scatter_impl(const void* send_buf,
                        void* recv_buf,
                        size_t recv_count,
                        ccl_datatype_t dtype,
                        ccl_reduction_t reduction,
                        const reduce_scatter_attr_t& attr);

    template <class BufferType,
              class = typename std::enable_if<ccl::is_native_type_supported<BufferType>()>::type>
    ccl::communicator::request_t
    reduce_scatter_impl(const BufferType* send_buf,
                        BufferType* recv_buf,
                        size_t recv_count,
                        ccl_reduction_t reduction,
                        const reduce_scatter_attr_t& attr);

    /* sparse_allreduce */
    ccl::communicator::request_t
    sparse_allreduce_impl(const void* send_ind_buf,
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
                          const sparse_allreduce_attr_t& attr);

    template <
        class index_BufferType,
        class value_BufferType,
        class = typename std::enable_if<ccl::is_native_type_supported<value_BufferType>()>::type>
    ccl::communicator::request_t
    sparse_allreduce_impl(const index_BufferType* send_ind_buf,
                          size_t send_ind_count,
                          const value_BufferType* send_val_buf,
                          size_t send_val_count,
                          index_BufferType* recv_ind_buf,
                          size_t recv_ind_count,
                          value_BufferType* recv_val_buf,
                          size_t recv_val_count,
                          ccl_reduction_t reduction,
                          const sparse_allreduce_attr_t& attr);

    host_communicator();
    host_communicator(size_t size,
                      shared_ptr_class<kvs_interface> kvs);
    host_communicator(size_t size,
                      size_t rank,
                      shared_ptr_class<kvs_interface> kvs);
    host_communicator(host_communicator& src) = delete;
    host_communicator(host_communicator&& src) = default;
    host_communicator& operator=(host_communicator& src) = delete;
    host_communicator& operator=(host_communicator&& src) = default;
    ~host_communicator() = default;

private:
    size_t comm_rank;
    size_t comm_size;
}; // class host_communicator

} // namespace ccl
