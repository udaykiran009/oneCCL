#pragma once

#ifndef CCL_PRODUCT_FULL
#error "Do not include this file directly. Please include 'ccl.hpp'"
#endif

#if defined(MULTI_GPU_SUPPORT) || defined(CCL_ENABLE_SYCL)
namespace ccl {
class request;
class kvs_interface;
using rank_t = size_t;

struct communicator_interface;
/**
 * A device communicator that permits device communication operations
 * Has no defined public constructor.
 * Use ccl::environment::create_device_communicator for communicator objects creation.
 */
class device_communicator final : public ccl_api_base_movable<device_communicator, direct_access_policy, communicator_interface, std::shared_ptr>
{
public:
    using base_t = ccl_api_base_movable<device_communicator, direct_access_policy, communicator_interface, std::shared_ptr>;

    /**
     * Declare PIMPL type
     */
    using impl_value_t = typename base_t::impl_value_t;

    /**
     * Declare implementation type
     */
    using impl_t = typename impl_value_t::element_type;

    /**
     * Type allows to get underlying device type,
     * which was used as communicator construction argument
     */
    using ccl_device_t = typename unified_device_type::ccl_native_t;

    /**
     * Declare communicator device context native type
     */
    using ccl_context_t = typename unified_device_context_type::ccl_native_t;


    using request_t = ccl::request_t;
    using coll_request_t = request_t;

    device_communicator(device_communicator&& src);
    device_communicator& operator=(device_communicator&& src);
    ~device_communicator();

 /**
     * Retrieves the rank in a communicator
     * @return rank corresponding to communicator object
     */
    size_t rank() const;

    /**
     * Retrieves the number of rank in a communicator
     * @return number of the ranks
     */
    size_t size() const;

    /**
     * Retrieves underlying device, which was used as communicator construction argument
     */
    ccl_device_t get_device();

    /**
     * Retrieves underlying context, which was used as communicator construction argument
     */
    ccl_context_t get_context();


    size_t get_group_unique_id() const;


    bool is_ready() const;


    template <class ...attr_value_pair_t>
    stream create_stream(attr_value_pair_t&&...avps)
    {
        return create_stream_from_attr(get_device(), get_context(), std::forward<attr_value_pair_t>(avps)...);
    }


    device_communicator split(const device_comm_split_attr_t& attr);


    /**
     * Allgatherv is a collective communication operation that collects data from all ranks within a communicator
     * into a single buffer or vector of buffers, one per rank. Different ranks can contribute segments of different sizes.
     * The resulting data in the output buffer(s) must be the same for each rank.
     */

    /**
     * @param send_buf the buffer with @c send_count elements of @c dtype that stores local data to be gathered
     * @param send_count number of elements of type @c dtype in @c send_buf
     * @param recv_buf [out] the buffer to store gathered result, should be large enough to hold values from all ranks
     * @param recv_counts array with number of elements of type @c dtype to be received from each rank
     * @param dtype datatype of elements in @c send_buf and @c recv_buf
     * @param op_stream op_stream associated with the operation
     * @param deps optional vector of events that the operation should depend on
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    request_t allgatherv(const void* send_buf,
                         size_t send_count,
                         void* recv_buf,
                         const vector_class<size_t>& recv_counts,
                         datatype dtype,
                         const allgatherv_attr_t& attr = default_allgather_attr,
                         stream op_stream = default_stream,
                         const vector_class<event>& deps = {});

    /**
     * @param send_buf the buffer with @c send_count elements of @c dtype that stores local data to be gathered
     * @param send_count number of elements of type @c dtype in @c send_buf
     * @param recv_bufs [out] array of buffers to store gathered result, one buffer per each rank
     * @param recv_counts array with number of elements of type @c dtype to be received from each rank
     * @param dtype datatype of elements in @c send_buf and @c recv_buf
     * @param op_stream op_stream associated with the operation
     * @param deps optional vector of events that the operation should depend on
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    request_t allgatherv(const void* send_buf,
                         size_t send_count,
                         const vector_class<void*>& recv_bufs,
                         const vector_class<size_t>& recv_counts,
                         datatype dtype,
                         const allgatherv_attr_t& attr = default_allgather_attr,
                         stream op_stream = default_stream,
                         const vector_class<event>& deps = {});
    /**
     * Type safety version:
     * @param send_buf the buffer with @c send_count elements of @c BufferType that stores local data to be gathered
     * @param send_count number of elements of type @c BufferType in @c send_buf
     * @param recv_buf [out] the buffer to store gathered result, should be large enough to hold values from all ranks
     * @param recv_counts array with number of elements of type @c BufferType to be received from each rank
     * @param op_stream op_stream associated with the operation
     * @param deps optional vector of events that the operation should depend on
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    template <class BufferType,
              class = typename std::enable_if<ccl::is_native_type_supported<BufferType>()>::type>
    request_t allgatherv(const BufferType* send_buf,
                         size_t send_count,
                         BufferType* recv_buf,
                         const vector_class<size_t>& recv_counts,
                         const allgatherv_attr_t& attr/* = allgatherv_attr_t()*/,
                         stream op_stream,
                         const vector_class<event>& deps = {});

    /**
     * Type safety version:
     * @param send_buf the buffer with @c send_count elements of @c BufferType that stores local data to be gathered
     * @param send_count number of elements of type @c BufferType in @c send_buf
     * @param recv_bufs [out] array of buffers to store gathered result, one buffer per each rank
     * @param recv_counts array with number of elements of type @c BufferType to be received from each rank
     * @param op_stream op_stream associated with the operation
     * @param deps optional vector of events that the operation should depend on
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    template <class BufferType,
              class = typename std::enable_if<ccl::is_native_type_supported<BufferType>()>::type>
    request_t allgatherv(const BufferType* send_buf,
                         size_t send_count,
                         vector_class<BufferType*>& recv_bufs,
                         const vector_class<size_t>& recv_counts,
                         const allgatherv_attr_t& attr/* = allgatherv_attr_t()*/,
                         stream op_stream,
                         const vector_class<event>& deps = {});

    /**
     * Type safety version:
     * @param send_buf the buffer with @c send_count elements of @c dtype that stores local data to be gathered
     * @param send_count number of elements of type @c dtype in @c send_buf
     * @param recv_buf [out] the buffer to store gathered result, should be large enough to hold values from all ranks
     * @param recv_counts array with number of elements of type @c dtype to be received from each rank
     * @param op_stream op_stream associated with the operation
     * @param deps optional vector of events that the operation should depend on
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    template <class BufferObjectType,
              class = typename std::enable_if<ccl::is_class_supported<BufferObjectType>()>::type>
    request_t allgatherv(const BufferObjectType& send_buf,
                         size_t send_count,
                         BufferObjectType& recv_buf,
                         const vector_class<size_t>& recv_counts,
                         const allgatherv_attr_t& attr/* = allgatherv_attr_t()*/,
                         stream op_stream,
                         const vector_class<event>& deps = {});

    /**
     * Type safety version:
     * @param send_buf the buffer with @c send_count elements of @c dtype that stores local data to be gathered
     * @param send_count number of elements of type @c dtype in @c send_buf
     * @param recv_bufs [out] array of buffers to store gathered result, one buffer per each rank
     * @param recv_counts array with number of elements of type @c dtype to be received from each rank
     * @param op_stream op_stream associated with the operation
     * @param deps optional vector of events that the operation should depend on
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    template <class BufferObjectType,
              class = typename std::enable_if<ccl::is_class_supported<BufferObjectType>()>::type>
    request_t allgatherv(const BufferObjectType& send_buf,
                         size_t send_count,
                         vector_class<BufferObjectType&>& recv_bufs,
                         const vector_class<size_t>& recv_counts,
                         const allgatherv_attr_t& attr/* = allgatherv_attr_t()*/,
                         stream op_stream,
                         const vector_class<event>& deps = {});

    /**
     * Allreduce is a collective communication operation that makes global reduction operation
     * on values from all ranks of communicator and distributes result back to all ranks.
     */

    /**
     * @param send_buf the buffer with @c count elements of @c dtype that stores local data to be reduced
     * @param recv_buf [out] the buffer to store reduced result, must have the same dimension as @c send_buf
     * @param count number of elements of type @c dtype in @c send_buf and @c recv_buf
     * @param dtype datatype of elements in @c send_buf and @c recv_buf
     * @param reduction type of reduction operation to be applied
     * @param op_stream op_stream associated with the operation
     * @param deps optional vector of events that the operation should depend on
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    request_t allreduce(const void* send_buf,
                        void* recv_buf,
                        size_t count,
                        datatype dtype,
                        reduction reduction,
                        const allreduce_attr_t& attr,/* = allreduce_attr_t()*/
                        stream op_stream,
                        const vector_class<event>& deps = {});

    /**
     * Type safety version:
     * @param send_buf the buffer with @c count elements of @c BufferType that stores local data to be reduced
     * @param recv_buf [out] the buffer to store reduced result, must have the same dimension as @c send_buf
     * @param count number of elements of type @c BufferType in @c send_buf and @c recv_buf
     * @param reduction type of reduction operation to be applied
     * @param op_stream op_stream associated with the operation
     * @param deps optional vector of events that the operation should depend on
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    template <class BufferType,
              class = typename std::enable_if<ccl::is_native_type_supported<BufferType>()>::type>
    request_t allreduce(const BufferType* send_buf,
                        BufferType* recv_buf,
                        size_t count,
                        reduction reduction,
                        const allreduce_attr_t& attr,/* = allreduce_attr_t()*/
                        stream op_stream,
                        const vector_class<event>& deps = {});

    /**
     * Type safety version:
     * @param send_buf the buffer with @c count elements of @c dtype that stores local data to be reduced
     * @param recv_buf [out] the buffer to store reduced result, must have the same dimension as @c send_buf
     * @param count number of elements of type @c dtype in @c send_buf and @c recv_buf
     * @param reduction type of reduction operation to be applied
     * @param op_stream op_stream associated with the operation
     * @param deps optional vector of events that the operation should depend on
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    template <class BufferObjectType,
              class = typename std::enable_if<ccl::is_class_supported<BufferObjectType>()>::type>
    request_t allreduce(const BufferObjectType& send_buf,
                        BufferObjectType& recv_buf,
                        size_t count,
                        reduction reduction,
                        const allreduce_attr_t& attr/* = allreduce_attr_t()*/,
                        stream op_stream,
                        const vector_class<event>& deps = {});

    /**
     * Alltoall is a collective communication operation in which each rank
     * sends distinct equal-sized blocks of data to each rank.
     * The j-th block of @c send_buf sent from the i-th rank is received by the j-th rank
     * and is placed in the i-th block of @c recvbuf.
     */

    /**
     * @param send_buf the buffer with @c count elements of @c dtype that stores local data to be sent
     * @param recv_buf [out] the buffer to store received result, should be large enough
     * to hold values from all ranks, i.e. at least @c comm_size * @c count
     * @param count number of elements of type @c dtype to be send to or to received from each rank
     * @param dtype datatype of elements in @c send_buf and @c recv_buf
     * @param op_stream op_stream associated with the operation
     * @param deps optional vector of events that the operation should depend on
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    request_t alltoall(const void* send_buf,
                       void* recv_buf,
                       size_t count,
                       datatype dtype,
                       const alltoall_attr_t& attr/* = alltoall_attr_t()*/,
                       stream op_stream,
                       const vector_class<event>& deps = {});

    /**
     * @param send_bufs array of buffers with local data to be sent, one buffer per each rank
     * @param recv_bufs [out] array of buffers to store received result, one buffer per each rank
     * @param count number of elements of type @c dtype to be send to or to received from each rank
     * @param dtype datatype of elements in @c send_buf and @c recv_buf
     * @param op_stream op_stream associated with the operation
     * @param deps optional vector of events that the operation should depend on
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    request_t alltoall(const vector_class<void*>& send_buf,
                       const vector_class<void*>& recv_buf,
                       size_t count,
                       datatype dtype,
                       const alltoall_attr_t& attr/* = alltoall_attr_t()*/,
                       stream op_stream,
                       const vector_class<event>& deps = {});

    /**
     * Type safety version:
     * @param send_buf the buffer with @c count elements of @c BufferType that stores local data to be sent
     * @param recv_buf [out] the buffer to store received result, should be large enough
     * to hold values from all ranks, i.e. at least @c comm_size * @c count
     * @param count number of elements to be send to or to received from each rank
     * @param op_stream op_stream associated with the operation
     * @param deps optional vector of events that the operation should depend on
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    template <class BufferType,
              class = typename std::enable_if<ccl::is_native_type_supported<BufferType>()>::type>
    request_t alltoall(const BufferType* send_buf,
                       BufferType* recv_buf,
                       size_t count,
                       const alltoall_attr_t& attr/* = alltoall_attr_t()*/,
                       stream op_stream,
                       const vector_class<event>& deps = {});

    /**
     * Type safety version:
     * @param send_bufs array of buffers with local data to be sent, one buffer per each rank
     * @param recv_bufs [out] array of buffers to store received result, one buffer per each rank
     * @param count number of elements to be send to or to received from each rank
     * @param op_stream op_stream associated with the operation
     * @param deps optional vector of events that the operation should depend on
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    template <class BufferType,
              class = typename std::enable_if<ccl::is_native_type_supported<BufferType>()>::type>
    request_t alltoall(const vector_class<BufferType*>& send_buf,
                       const vector_class<BufferType*>& recv_buf,
                       size_t count,
                       const alltoall_attr_t& attr/* = alltoall_attr_t()*/,
                       stream op_stream,
                       const vector_class<event>& deps = {});

    /**
     * Type safety version:
     * @param send_buf the buffer with @c count elements of @c dtype that stores local data to be sent
     * @param recv_buf [out] the buffer to store received result, should be large enough
     * to hold values from all ranks, i.e. at least @c comm_size * @c count
     * @param count number of elements to be send to or to received from each rank
     * @param op_stream op_stream associated with the operation
     * @param deps optional vector of events that the operation should depend on
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    template <class BufferObjectType,
              class = typename std::enable_if<ccl::is_class_supported<BufferObjectType>()>::type>
    request_t alltoall(const BufferObjectType& send_buf,
                       BufferObjectType& recv_buf,
                       size_t count,
                       const alltoall_attr_t& attr/* = alltoall_attr_t()*/,
                       stream op_stream,
                       const vector_class<event>& deps = {});

    /**
     * Type safety version:
     * @param send_bufs array of buffers with local data to be sent, one buffer per each rank
     * @param recv_bufs [out] array of buffers to store received result, one buffer per each rank
     * @param count number of elements to be send to or to received from each rank
     * @param op_stream op_stream associated with the operation
     * @param deps optional vector of events that the operation should depend on
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    template <class BufferObjectType,
              class = typename std::enable_if<ccl::is_class_supported<BufferObjectType>()>::type>
    request_t alltoall(const vector_class<BufferObjectType&>& send_buf,
                       const vector_class<BufferObjectType&>& recv_buf,
                       size_t count,
                       const alltoall_attr_t& attr/* = alltoall_attr_t()*/,
                       stream op_stream,
                       const vector_class<event>& deps = {});

    /**
     * Alltoallv is a collective communication operation in which each rank
     * sends distinct blocks of data to each rank. Block sizes may differ.
     * The j-th block of @c send_buf sent from the i-th rank is received by the j-th rank
     * and is placed in the i-th block of @c recvbuf.
     */

    /**
     * @param send_buf the buffer with elements of @c dtype that stores local blocks to be sent to each rank
     * @param send_counts array with number of elements of type @c dtype in send blocks for each rank
     * @param recv_buf [out] the buffer to store received result, should be large enough to hold blocks from all ranks
     * @param recv_counts array with number of elements of type @c dtype in receive blocks from each rank
     * @param dtype datatype of elements in @c send_buf and @c recv_buf
     * @param op_stream op_stream associated with the operation
     * @param deps optional vector of events that the operation should depend on
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    request_t alltoallv(const void* send_buf,
                        const vector_class<size_t>& send_counts,
                        void* recv_buf,
                        const vector_class<size_t>& recv_counts,
                        datatype dtype,
                        const alltoallv_attr_t& attr/* = alltoallv_attr_t()*/,
                        stream op_stream,
                        const vector_class<event>& deps = {});

    /**
     * @param send_bufs array of buffers to store send blocks, one buffer per each rank
     * @param send_counts array with number of elements of type @c dtype in send blocks for each rank
     * @param recv_bufs [out] array of buffers to store receive blocks, one buffer per each rank
     * @param recv_counts array with number of elements of type @c dtype in receive blocks from each rank
     * @param dtype datatype of elements in @c send_buf and @c recv_buf
     * @param op_stream op_stream associated with the operation
     * @param deps optional vector of events that the operation should depend on
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    request_t alltoallv(const vector_class<void*>& send_bufs,
                        const vector_class<size_t>& send_counts,
                        const vector_class<void*>& recv_bufs,
                        const vector_class<size_t>& recv_counts,
                        datatype dtype,
                        const alltoallv_attr_t& attr/* = alltoallv_attr_t()*/,
                        stream op_stream,
                        const vector_class<event>& deps = {});

    /**
     * Type safety version:
     * @param send_buf the buffer with elements of @c BufferType that stores local blocks to be sent to each rank
     * @param send_counts array with number of elements of type @c BufferType in send blocks for each rank
     * @param recv_buf [out] the buffer to store received result, should be large enough to hold blocks from all ranks
     * @param recv_counts array with number of elements of type @c BufferType in receive blocks from each rank
     * @param op_stream op_stream associated with the operation
     * @param deps optional vector of events that the operation should depend on
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    template <class BufferType,
              class = typename std::enable_if<ccl::is_native_type_supported<BufferType>()>::type>
    request_t alltoallv(const BufferType* send_buf,
                        const vector_class<size_t>& send_counts,
                        BufferType* recv_buf,
                        const vector_class<size_t>& recv_counts,
                        const alltoallv_attr_t& attr/* = alltoallv_attr_t()*/,
                        stream op_stream,
                        const vector_class<event>& deps = {});

    /**
     * Type safety version:
     * @param send_bufs array of buffers to store send blocks, one buffer per each rank
     * @param send_counts array with number of elements of type @c BufferType in send blocks for each rank
     * @param recv_bufs [out] array of buffers to store receive blocks, one buffer per each rank
     * @param recv_counts array with number of elements of type @c BufferType in receive blocks from each rank
     * @param op_stream op_stream associated with the operation
     * @param deps optional vector of events that the operation should depend on
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    template <class BufferType,
              class = typename std::enable_if<ccl::is_native_type_supported<BufferType>()>::type>
    request_t alltoallv(const vector_class<BufferType*>& send_bufs,
                        const vector_class<size_t>& send_counts,
                        const vector_class<BufferType*>& recv_bufs,
                        const vector_class<size_t>& recv_counts,
                        const alltoallv_attr_t& attr/* = alltoallv_attr_t()*/,
                        stream op_stream,
                        const vector_class<event>& deps = {});

    /**
     * Type safety version:
     * @param send_buf the buffer with elements of @c dtype that stores local blocks to be sent to each rank
     * @param send_counts array with number of elements of type @c dtype in send blocks for each rank
     * @param recv_buf [out] the buffer to store received result, should be large enough to hold blocks from all ranks
     * @param recv_counts array with number of elements of type @c dtype in receive blocks from each rank
     * @param op_stream op_stream associated with the operation
     * @param deps optional vector of events that the operation should depend on
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    template <class BufferObjectType,
              class = typename std::enable_if<ccl::is_class_supported<BufferObjectType>()>::type>
    request_t alltoallv(const BufferObjectType& send_buf,
                        const vector_class<size_t>& send_counts,
                        BufferObjectType& recv_buf,
                        const vector_class<size_t>& recv_counts,
                        const alltoallv_attr_t& attr/* = alltoallv_attr_t()*/,
                        stream op_stream,
                        const vector_class<event>& deps = {});

    /**
     * Type safety version:
     * @param send_bufs array of buffers to store send blocks, one buffer per each rank
     * @param send_counts array with number of elements of type @c dtype in send blocks for each rank
     * @param recv_bufs [out] array of buffers to store receive blocks, one buffer per each rank
     * @param recv_counts array with number of elements of type @c dtype in receive blocks from each rank
     * @param dtype datatype of elements in @c send_buf and @c recv_buf
     * @param op_stream op_stream associated with the operation
     * @param deps optional vector of events that the operation should depend on
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    template <class BufferObjectType,
              class = typename std::enable_if<ccl::is_class_supported<BufferObjectType>()>::type>
    request_t alltoallv(const vector_class<BufferObjectType&>& send_bufs,
                        const vector_class<size_t>& send_counts,
                        const vector_class<BufferObjectType&>& recv_bufs,
                        const vector_class<size_t>& recv_counts,
                        const alltoallv_attr_t& attr/* = alltoallv_attr_t()*/,
                        stream op_stream,
                        const vector_class<event>& deps = {});

    /**
     * Barrier synchronization across all ranks of communicator.
     * Completes after all ranks in the communicator have called it.
     *
     * @param op_stream op_stream associated with the operation
     * @param deps optional vector of events that the operation should depend on
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    request_t barrier(const barrier_attr_t& attr/* = barrier_attr_t()*/,
                      stream op_stream,
                      const vector_class<event>& deps = {});

    /**
     * Bcast is collective communication operation that broadcasts data
     * from one rank of communicator (denoted as root) to all other ranks.
     */

    /**
     * @param buf [in,out] the buffer with @c count elements of @c dtype
     * serves as send buffer for root and as receive buffer for other ranks
     * @param count number of elements of type @c dtype in @c buf
     * @param dtype datatype of elements in @c buf
     * @param root the rank that broadcasts @c buf
     * @param op_stream op_stream associated with the operation
     * @param deps optional vector of events that the operation should depend on
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    request_t bcast(void* buf,
                    size_t count,
                    datatype dtype,
                    size_t root,
                    const bcast_attr_t& attr/* = bcast_attr_t()*/,
                    stream op_stream,
                    const vector_class<event>& deps = {});

    /**
     * Type safety version:
     * @param buf [in,out] the buffer with @c count elements of @c BufferType
     * serves as send buffer for root and as receive buffer for other ranks
     * @param count number of elements of type @c BufferType in @c buf
     * @param root the rank that broadcasts @c buf
     * @param op_stream op_stream associated with the operation
     * @param deps optional vector of events that the operation should depend on
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    template <class BufferType,
              class = typename std::enable_if<ccl::is_native_type_supported<BufferType>()>::type>
    request_t bcast(BufferType* buf,
                    size_t count,
                    size_t root,
                    const bcast_attr_t& attr/* = bcast_attr_t()*/,
                    stream op_stream,
                    const vector_class<event>& deps = {});

    /**
     * Type safety version:
     * @param buf [in,out] the buffer with @c count elements of @c dtype
     * serves as send buffer for root and as receive buffer for other ranks
     * @param count number of elements of type @c dtype in @c buf
     * @param root the rank that broadcasts @c buf
     * @param op_stream op_stream associated with the operation
     * @param deps optional vector of events that the operation should depend on
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    template <class BufferObjectType,
              class = typename std::enable_if<ccl::is_class_supported<BufferObjectType>()>::type>
    request_t bcast(BufferObjectType& buf,
                    size_t count,
                    size_t root,
                    const bcast_attr_t& attr/* = bcast_attr_t()*/,
                    stream op_stream,
                    const vector_class<event>& deps = {});

    /**
     * Reduce is a collective communication operation that makes global reduction operation
     * on values from all ranks of communicator and returns result to root rank.
     */

    /**
     * @param send_buf the buffer with @c count elements of @c dtype that stores local data to be reduced
     * @param recv_buf [out] the buffer to store reduced result, must have the same dimension as @c send_buf.
     * Used by the @c root rank only, ignored by other ranks.
     * @param count number of elements of type @c dtype in @c send_buf and @c recv_buf
     * @param dtype datatype of elements in @c send_buf and @c recv_buf
     * @param reduction type of reduction operation to be applied
     * @param root the rank that gets the result of reduction
     * @param op_stream op_stream associated with the operation
     * @param deps optional vector of events that the operation should depend on
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    request_t reduce(const void* send_buf,
                     void* recv_buf,
                     size_t count,
                     datatype dtype,
                     reduction reduction,
                     size_t root,
                     const reduce_attr_t& attr/* = reduce_attr_t()*/,
                     stream op_stream,
                     const vector_class<event>& deps = {});

    /**
     * Type safety version:
     * @param send_buf the buffer with @c count elements of @c BufferType that stores local data to be reduced
     * @param recv_buf [out] the buffer to store reduced result, must have the same dimension as @c send_buf.
     * Used by the @c root rank only, ignored by other ranks.
     * @param count number of elements of type @c BufferType in @c send_buf and @c recv_buf
     * @param reduction type of reduction operation to be applied
     * @param root the rank that gets the result of reduction
     * @param op_stream op_stream associated with the operation
     * @param deps optional vector of events that the operation should depend on
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    template <class BufferType,
              class = typename std::enable_if<ccl::is_native_type_supported<BufferType>()>::type>
    request_t reduce(const BufferType* send_buf,
                     BufferType* recv_buf,
                     size_t count,
                     reduction reduction,
                     size_t root,
                     const reduce_attr_t& attr/* = reduce_attr_t()*/,
                     stream op_stream,
                     const vector_class<event>& deps = {});

    /**
     * Type safety version:
     * @param send_buf the buffer with @c count elements of @c dtype that stores local data to be reduced
     * @param recv_buf [out] the buffer to store reduced result, must have the same dimension as @c send_buf.
     * Used by the @c root rank only, ignored by other ranks.
     * @param count number of elements of type @c dtype in @c send_buf and @c recv_buf
     * @param dtype datatype of elements in @c send_buf and @c recv_buf
     * @param reduction type of reduction operation to be applied
     * @param op_stream op_stream associated with the operation
     * @param deps optional vector of events that the operation should depend on
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    template <class BufferObjectType,
              class = typename std::enable_if<ccl::is_class_supported<BufferObjectType>()>::type>
    request_t reduce(const BufferObjectType& send_buf,
                     BufferObjectType& recv_buf,
                     size_t count,
                     reduction reduction,
                     size_t root,
                     const reduce_attr_t& attr/* = reduce_attr_t()*/,
                     stream op_stream,
                     const vector_class<event>& deps = {});

    /**
     * Reduce-scatter is a collective communication operation that makes global reduction operation
     * on values from all ranks of communicator and scatters result in blocks back to all ranks.
     */

    /**
     * @param send_buf the buffer with @c comm_size * @c count elements of @c dtype that stores local data to be reduced
     * @param recv_buf [out] the buffer to store result block containing @c recv_count elements of type @c dtype
     * @param recv_count number of elements of type @c dtype in receive block
     * @param dtype datatype of elements in @c send_buf and @c recv_buf
     * @param reduction type of reduction operation to be applied
     * @param op_stream op_stream associated with the operation
     * @param deps optional vector of events that the operation should depend on
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    request_t reduce_scatter(const void* send_buf,
                             void* recv_buf,
                             size_t recv_count,
                             datatype dtype,
                             reduction reduction,
                             const reduce_scatter_attr_t& attr/* = reduce_scatter_attr_t()*/,
                             stream op_stream,
                             const vector_class<event>& deps = {});

    /**
     * Type safety version:
     * @param send_buf the buffer with @c comm_size * @c count elements of @c BufferType that stores local data to be reduced
     * @param recv_buf [out] the buffer to store result block containing @c recv_count elements of type @c BufferType
     * @param recv_count number of elements of type @c BufferType in receive block
     * @param reduction type of reduction operation to be applied
     * @param op_stream op_stream associated with the operation
     * @param deps optional vector of events that the operation should depend on
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    template <class BufferType,
              class = typename std::enable_if<ccl::is_native_type_supported<BufferType>()>::type>
    request_t reduce_scatter(const BufferType* send_buf,
                             BufferType* recv_buf,
                             size_t recv_count,
                             reduction reduction,
                             const reduce_scatter_attr_t& attr/* = reduce_scatter_attr_t()*/,
                             stream op_stream,
                             const vector_class<event>& deps = {});

    /**
     * Type safety version:
     * @param send_buf the buffer with @c comm_size * @c count elements of @c dtype that stores local data to be reduced
     * @param recv_buf [out] the buffer to store result block containing @c recv_count elements of type @c dtype
     * @param recv_count number of elements of type @c dtype in receive block
     * @param reduction type of reduction operation to be applied
     * @param op_stream op_stream associated with the operation
     * @param deps optional vector of events that the operation should depend on
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    // template <class BufferObjectType,
    //           class = typename std::enable_if<ccl::is_class_supported<BufferObjectType>()>::type>

    /* TODO: apply this in other places */
    template <class BufferObjectType,
              class = typename std::enable_if<ccl::is_class_supported<BufferObjectType>(), request_t>::type>
    request_t reduce_scatter(const BufferObjectType& send_buf,
                   BufferObjectType& recv_buf,
                   size_t recv_count,
                   reduction reduction,
                   const reduce_scatter_attr_t& attr/* = reduce_scatter_attr_t()*/,
                   stream op_stream,
                   const vector_class<event>& deps = {});

    /**
     * Sparse allreduce is a collective communication operation that makes global reduction operation
     * on sparse buffers from all ranks of communicator and distributes result back to all ranks.
     * Sparse buffers are defined by separate index and value buffers.
     */

    /**
     * @param send_ind_buf the buffer of indices with @c send_ind_count elements of type @c ind_dtype
     * @param send_int_count number of elements of type @c ind_type @c send_ind_buf
     * @param send_val_buf the buffer of values with @c send_val_count elements of type @c val_dtype
     * @param send_val_count number of elements of type @c val_type @c send_val_buf
     * @param recv_ind_buf [out] the buffer to store reduced indices, unused
     * @param recv_ind_count [out] number of elements in @c recv_ind_buf, unused
     * @param recv_val_buf [out] the buffer to store reduced values, unused
     * @param recv_val_count [out] number of elements in @c recv_val_buf, unused
     * @param ind_dtype datatype of elements in @c send_ind_buf and @c recv_ind_buf
     * @param val_dtype datatype of elements in @c send_val_buf and @c recv_val_buf
     * @param reduction type of reduction operation to be applied
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    request_t sparse_allreduce(const void* send_ind_buf,
                               size_t send_ind_count,
                               const void* send_val_buf,
                               size_t send_val_count,
                               void* recv_ind_buf,
                               size_t recv_ind_count,
                               void* recv_val_buf,
                               size_t recv_val_count,
                               datatype ind_dtype,
                               datatype val_dtype,
                               reduction reduction,
                               const sparse_allreduce_attr_t& attr/* = sparse_allreduce_attr_t()*/,
                               stream op_stream,
                               const vector_class<event>& deps = {});

    /**
     * Type safety version:
     * @param send_ind_buf the buffer of indices with @c send_ind_count elements of type @c ind_dtype
     * @param send_int_count number of elements of type @c ind_type @c send_ind_buf
     * @param send_val_buf the buffer of values with @c send_val_count elements of type @c val_dtype
     * @param send_val_count number of elements of type @c val_type @c send_val_buf
     * @param recv_ind_buf [out] the buffer to store reduced indices, unused
     * @param recv_ind_count [out] number of elements in @c recv_ind_buf, unused
     * @param recv_val_buf [out] the buffer to store reduced values, unused
     * @param recv_val_count [out] number of elements in @c recv_val_buf, unused
     * @param reduction type of reduction operation to be applied
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    template <
        class index_BufferType,
        class value_BufferType,
        class = typename std::enable_if<ccl::is_native_type_supported<value_BufferType>()>::type>
    request_t sparse_allreduce(const index_BufferType* send_ind_buf,
                               size_t send_ind_count,
                               const value_BufferType* send_val_buf,
                               size_t send_val_count,
                               index_BufferType* recv_ind_buf,
                               size_t recv_ind_count,
                               value_BufferType* recv_val_buf,
                               size_t recv_val_count,
                               reduction reduction,
                               const sparse_allreduce_attr_t& attr/* = sparse_allreduce_attr_t()*/,
                               stream op_stream,
                               const vector_class<event>& deps = {});

    /**
     * Type safety version:
     * @param send_ind_buf the buffer of indices with @c send_ind_count elements of type @c ind_dtype
     * @param send_int_count number of elements of type @c ind_type @c send_ind_buf
     * @param send_val_buf the buffer of values with @c send_val_count elements of type @c val_dtype
     * @param send_val_count number of elements of type @c val_type @c send_val_buf
     * @param recv_ind_buf [out] the buffer to store reduced indices, unused
     * @param recv_ind_count [out] number of elements in @c recv_ind_buf, unused
     * @param recv_val_buf [out] the buffer to store reduced values, unused
     * @param recv_val_count [out] number of elements in @c recv_val_buf, unused
     * @param reduction type of reduction operation to be applied
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    template <
        class index_BufferObjectType,
        class value_BufferObjectType,
        class = typename std::enable_if<ccl::is_native_type_supported<value_BufferObjectType>()>::type>
    request_t sparse_allreduce(const index_BufferObjectType& send_ind_buf,
                               size_t send_ind_count,
                               const value_BufferObjectType& send_val_buf,
                               size_t send_val_count,
                               index_BufferObjectType& recv_ind_buf,
                               size_t recv_ind_count,
                               value_BufferObjectType& recv_val_buf,
                               size_t recv_val_count,
                               reduction reduction,
                               const sparse_allreduce_attr_t& attr/* = sparse_allreduce_attr_t()*/,
                               stream op_stream,
                               const vector_class<event>& deps = {});
private:
    friend class environment;
    friend class comm_group;
    device_communicator(impl_value_t&& impl);

    // factory methods
    template<class DeviceType,
         class ContextType>
    static vector_class<device_communicator> create_device_communicators(
        const size_t cluster_devices_size,
        const vector_class<DeviceType>& local_devices,
        ContextType& context,
        shared_ptr_class<kvs_interface> kvs);

    template<class DeviceType,
         class ContextType>
    static vector_class<device_communicator> create_device_communicators(
        const size_t cluster_devices_size, /*global devics count*/
        const vector_class<pair_class<rank_t, DeviceType>>& local_rank_device_map,
        ContextType& context,
        shared_ptr_class<kvs_interface> kvs);

    template<class DeviceType,
         class ContextType>
    static vector_class<device_communicator> create_device_communicators(
        const size_t cluster_devices_size, /*global devics count*/
        const map_class<rank_t, DeviceType>& local_rank_device_map,
        ContextType& context,
        shared_ptr_class<kvs_interface> kvs);
};



} // namespace ccl
#endif //#if defined(MULTI_GPU_SUPPORT) || defined(CCL_ENABLE_SYCL)
