#pragma once

#include <memory>
#include <ostream>
#include <utility>
#include <vector>

#include "ccl_attr.hpp"
#include "ccl_types.hpp"
#include "ccl_type_traits.hpp"

#include "ccl_stream.hpp"

class ccl_comm;
class ccl_event;
class ccl_stream;

namespace ccl {

class stream;
class communicator;
struct communicator_interface;

struct comm_split_attr_impl;
class comm_split_attr;

#ifdef DEVICE_COMM_SUPPORT
class device_comm_group;
class device_communicator;
struct device_comm_split_attr_impl;
class device_comm_split_attr;
#endif /* DEVICE_COMM_SUPPORT */

template <class T, class Alloc = std::allocator<T>>
using vector_class = std::vector<T, Alloc>;

template <class T, std::size_t N>
using array_class = std::array<T, N>;

using string_class = std::string;

template <class R, class... ArgTypes>
using function_class = std::function<R(ArgTypes...)>;

using mutex_class = std::mutex;

template <class T1, class T2>
using pair_class = std::pair<T1, N2>;

template <class... Types>
using tuple_class = std::tuple<Types...>;

template <class T>
using shared_ptr_class = std::shared_ptr<T>;

template <class T>
using unique_ptr_class = std::unique_ptr<T>;

/**
 * Types which allow to operate with kvs/communicator/request/stream/event objects in RAII manner
 */
using kvs_interface_t = unique_ptr_class<kvs_interface>;
using communicator_t = unique_ptr_class<communicator>;
using request_t = unique_ptr_class<request>;
using event_t = unique_ptr_class<event>;

#ifdef DEVICE_COMM_SUPPORT
using device_communicator_t = unique_ptr_class<device_communicator>;
using stream_t = unique_ptr_class<stream>;
#endif /* DEVICE_COMM_SUPPORT */

class kvs_interface {
public:
    virtual vector_class<char> get(const string_class& prefix, const string_class& key) const = 0;

    virtual void set(const string_class& prefix,
                     const string_class& key,
                     const vector_class<char>& data) const = 0;

    virtual ~kvs_interface() = default;
};

class kvs final : public kvs_interface {
public:
    static constexpr size_t addr_max_size = 256;
    using addr_t = array_class<char, addr_max_size>;

    const addr_t& get_addr() const;

    ~kvs() override;

    vector_class<char> get(const string_class& prefix, const string_class& key) const override;

    void set(const string_class& prefix,
             const string_class& key,
             const vector_class<char>& data) const override;

private:
    kvs();
    kvs(const addr_t& addr);

    unique_ptr_class<kvs_impl> pimpl;
};

/**
 * CCL environment singleton
 */
class environment {
public:
    ~environment();

    /**
     * Retrieves the unique environment object
     * and makes the first-time initialization of CCL library
     */
    static environment& instance();

    /**
     * Retrieves the current version
     */
    version_t get_version() const;

    /**
     * Creates @attr which used to register custom datatype
     */
    datatype_attr_t create_datatype_attr() const;

    /**
     * Registers custom datatype to be used in communication operations
     * @param attr datatype attributes
     * @return datatype handle
     */
    datatype register_datatype(const datatype_attr_t& attr);

    /**
     * Deregisters custom datatype
     * @param dtype custom datatype handle
     */
    void deregister_datatype(datatype dtype);

    /**
     * Retrieves a datatype size in bytes
     * @param dtype datatype handle
     * @return datatype size
     */
    size_t get_datatype_size(datatype dtype) const;

    /**
     * Creates a main key-value store.
     * It's address should be distributed using out of band communication mechanism
     * and be used to create key-value stores on other ranks.
     * @return kvs object
     */
    kvs_t create_main_kvs() const;

    /**
     * Creates a new key-value store from main kvs address
     * @param addr address of main kvs
     * @return kvs object
     */
    kvs_t create_kvs(const kvs::addr_t& addr) const;

    /**
     * Creates a new host communicator with externally provided size, rank and kvs.
     * Implementation is platform specific and non portable.
     * @return host communicator
     */
    communicator_t create_communicator() const;

    /**
     * Creates a new host communicator with user supplied size and kvs.
     * Rank will be assigned automatically.
     * @param size user-supplied total number of ranks
     * @param kvs key-value store for ranks wire-up
     * @return host communicator
     */
    communicator_t create_communicator(const size_t size,
                                       shared_ptr_class<kvs_interface> kvs) const;

    /**
     * Creates a new host communicator with user supplied size, rank and kvs.
     * @param size user-supplied total number of ranks
     * @param rank user-supplied rank
     * @param kvs key-value store for ranks wire-up
     * @return host communicator
     */
    communicator_t create_communicator(const size_t size,
                                       const size_t rank,
                                       shared_ptr_class<kvs_interface> kvs) const;

    /**
     * Creates @attr which used to split host communicator
     */
    comm_split_attr_t create_comm_split_attr() const;

#ifdef DEVICE_COMM_SUPPORT

    /**
     * Creates a new device communicators with user supplied size, device indices and kvs.
     * Ranks will be assigned automatically.
     * @param size user-supplied total number of ranks
     * @param devices user-supplied device objects for local ranks
     * @param context context containing the devices
     * @param kvs key-value store for ranks wire-up
     * @return vector of device communicators
     */
    vector_class<device_communicator_t> create_device_communicators(
        const size_t size,
        const vector_class<native_device_type>& devices,
        native_context_type& context,
        shared_ptr_class<kvs_interface> kvs) const;

    /**
     * Creates a new device communicators with user supplied size, ranks, device indices and kvs.
     * @param size user-supplied total number of ranks
     * @param rank_device_map user-supplied mapping of local ranks on devices
     * @param context context containing the devices
     * @param kvs key-value store for ranks wire-up
     * @return vector of device communicators
     */
    vector_class<device_communicator_t> create_device_communicators(
        const size_t size,
        vector_class<pair_class<size_t, native_device_type>>& rank_device_map,
        native_context_type& context,
        shared_ptr_class<kvs_interface> kvs) const;

    /**
     * Creates @attr which used to split device communicator
     */
    device_comm_split_attr_t create_device_comm_split_attr() const;

    /**
     * Splits device communicators according to attributes.
     * @param attrs split attributes for local communicators
     * @return vector of device communicators
     */
    vector_class<device_communicator_t> split_device_communicators(
        const vector_class<pair_class<device_communicator_t, device_comm_split_attr_t>>& attrs)
        const;

    /**
     * Creates a new stream from @native_stream_type
     * @param native_stream the existing handle of stream
     * @return stream object
     */
    template <class native_stream_type,
              class = typename std::enable_if<is_stream_supported<native_stream_type>()>::type>
    stream_t create_stream(native_stream_type& native_stream) const;

    /**
     * Creates a new event from @native_event_type
     * @param native_event the existing handle of event
     * @return event object
     */
    template <class native_event_type,
              class = typename std::enable_if<is_event_supported<native_event_type>()>::type>
    event_t create_event(native_event_type& native_event) const;

    // /**
    //  * Creates a new native_stream from @native_stream_type
    //  * @param native_stream the existing handle of stream
    //  * @props are optional specific properties
    //  */
    // template <class native_stream_type,
    //           class = typename std::enable_if<is_stream_supported<native_stream_type>()>::type,
    //           info::stream_properties... prop_ids>
    // stream_t create_stream(info::arg<prop_ids, info::stream_property_value_t>... props) const {
    //     using tuple_t = tuple_class<info::stream_property_value_t>... > ;
    //     tuple_t args{ props... };

    //     info::stream_properties_data_t stream_args;
    //     arg_filler exec(args);
    //     ccl_tuple_for_each(args, arg_filler);
    //     return create_event(native_stream, stream_args);
    // }

    // /* Example:
    //  *  auto stream = ccl::environment::instance().create_stream(stream_arg(info::stream_atts::ordinal, 0),
    //  *                                                           stream_arg(info::stream_properties::index, 1),
    //  *                                                           stream_arg(info::stream_properties::mode, ZE_ASYNC));
    //  *
    //  */
    // stream_t create_stream() const;

#endif /* DEVICE_COMM_SUPPORT */

private:
    // stream_t create_stream(
    //     const info::stream_properties_data_t& args = info::stream_properties_data_t{});

    environment();
};

/**
 * Request's interface that allows users to track communication operation progress
 */
class request {
public:
    /**
     * Blocking wait for operation completion
     */
    virtual void wait() = 0;

    /**
     * Non-blocking check for operation completion
     * @retval true if the operation has been completed
     * @retval false if the operation has not been completed
     */
    virtual bool test() = 0;

    /**
     * Cancel a pending asynchronous operation
     * @retval true if the operation has been canceled
     * @retval false if the operation has not been canceled
     */
    virtual bool cancel() = 0;

    /**
      * Retrieve event object to be used for synchronization
      * with computation or other communication operations
      * @return event object
      */
    virtual event_t get_event() = 0;

    virtual ~request() = default;
};

/**
 * A communicator that permits communication operations
 * Has no defined public constructor.
 * Use ccl::environment::create_communicator for communicator objects creation.
 */
class communicator final {
public:
    ~communicator();

    /**
     * Retrieves the rank in a communicator
     * @return rank corresponding to communicator object
     */
    size_t rank() const;

    /**
     * Retrieves the number of ranks in a communicator
     * @return number of the ranks
     */
    size_t size() const;

    communicator_t split(const comm_split_attr& attr);

    /**
     * Cancel all pending asynchronous operations in current communicator.
     * @retval number of successfully canceled operations
     */
    size_t cancel();

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
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    request_t allgatherv(const void* send_buf,
                         size_t send_count,
                         void* recv_buf,
                         const vector_class<size_t>& recv_counts,
                         datatype dtype,
                         const allgatherv_attr_t& attr = allgatherv_attr_t());

    /* TODO */
    /*

    allreduce_attr_t attr = env.create_allreduce_attr(
        info::op_attr(info::allreduce_attr_id::reduction_fn, fn),
        info::op_attr(info::operation_attr_id::match_id, "tensor_name"));
    */

    /*info::operation_attr_id::match_id
    info::allreduce_attr_id
    info::bcast_attr_id*/

    /**
     * @param send_buf the buffer with @c send_count elements of @c dtype that stores local data to be gathered
     * @param send_count number of elements of type @c dtype in @c send_buf
     * @param recv_bufs [out] array of buffers to store gathered result, one buffer per each rank
     * @param recv_counts array with number of elements of type @c dtype to be received from each rank
     * @param dtype datatype of elements in @c send_buf and @c recv_buf
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    request_t allgatherv(const void* send_buf,
                         size_t send_count,
                         const vector_class<void*>& recv_bufs,
                         const vector_class<size_t>& recv_counts,
                         datatype dtype,
                         const allgatherv_attr_t& attr = allgatherv_attr_t());
    /**
     * Type safety version:
     * @param send_buf the buffer with @c send_count elements of @c BufferType that stores local data to be gathered
     * @param send_count number of elements of type @c BufferType in @c send_buf
     * @param recv_buf [out] the buffer to store gathered result, should be large enough to hold values from all ranks
     * @param recv_counts array with number of elements of type @c BufferType to be received from each rank
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    template <class BufferType,
              class = typename std::enable_if<ccl::is_native_type_supported<BufferType>()>::type>
    request_t allgatherv(const BufferType* send_buf,
                         size_t send_count,
                         BufferType* recv_buf,
                         const vector_class<size_t>& recv_counts,
                         const allgatherv_attr_t& attr = allgatherv_attr_t());

    /**
     * Type safety version:
     * @param send_buf the buffer with @c send_count elements of @c BufferType that stores local data to be gathered
     * @param send_count number of elements of type @c BufferType in @c send_buf
     * @param recv_bufs [out] array of buffers to store gathered result, one buffer per each rank
     * @param recv_counts array with number of elements of type @c BufferType to be received from each rank
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    template <class BufferType,
              class = typename std::enable_if<ccl::is_native_type_supported<BufferType>()>::type>
    request_t allgatherv(const BufferType* send_buf,
                         size_t send_count,
                         vector_class<BufferType*>& recv_bufs,
                         const vector_class<size_t>& recv_counts,
                         const allgatherv_attr_t& attr = allgatherv_attr_t());

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
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    request_t allreduce(const void* send_buf,
                        void* recv_buf,
                        size_t count,
                        datatype dtype,
                        reduction reduction,
                        const allreduce_attr_t& attr = allreduce_attr_t());

    /**
     * Type safety version:
     * @param send_buf the buffer with @c count elements of @c BufferType that stores local data to be reduced
     * @param recv_buf [out] the buffer to store reduced result, must have the same dimension as @c send_buf
     * @param count number of elements of type @c BufferType in @c send_buf and @c recv_buf
     * @param reduction type of reduction operation to be applied
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    template <class BufferType,
              class = typename std::enable_if<ccl::is_native_type_supported<BufferType>()>::type>
    request_t allreduce(const BufferType* send_buf,
                        BufferType* recv_buf,
                        size_t count,
                        reduction reduction,
                        const allreduce_attr_t& attr = allreduce_attr_t());

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
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    request_t alltoall(const void* send_buf,
                       void* recv_buf,
                       size_t count,
                       datatype dtype,
                       const alltoall_attr_t& attr = alltoall_attr_t());

    /**
     * @param send_bufs array of buffers with local data to be sent, one buffer per each rank
     * @param recv_bufs [out] array of buffers to store received result, one buffer per each rank
     * @param count number of elements of type @c dtype to be send to or to received from each rank
     * @param dtype datatype of elements in @c send_buf and @c recv_buf
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    request_t alltoall(const vector_class<void*>& send_buf,
                       const vector_class<void*>& recv_buf,
                       size_t count,
                       datatype dtype,
                       const alltoall_attr_t& attr = alltoall_attr_t());

    /**
     * Type safety version:
     * @param send_buf the buffer with @c count elements of @c BufferType that stores local data to be sent
     * @param recv_buf [out] the buffer to store received result, should be large enough
     * to hold values from all ranks, i.e. at least @c comm_size * @c count
     * @param count number of elements to be send to or to received from each rank
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    template <class BufferType,
              class = typename std::enable_if<ccl::is_native_type_supported<BufferType>()>::type>
    request_t alltoall(const BufferType* send_buf,
                       BufferType* recv_buf,
                       size_t count,
                       const alltoall_attr_t& attr = alltoall_attr_t());

    /**
     * Type safety version:
     * @param send_bufs array of buffers with local data to be sent, one buffer per each rank
     * @param recv_bufs [out] array of buffers to store received result, one buffer per each rank
     * @param count number of elements to be send to or to received from each rank
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    template <class BufferType,
              class = typename std::enable_if<ccl::is_native_type_supported<BufferType>()>::type>
    request_t alltoall(const vector_class<BufferType*>& send_buf,
                       const vector_class<BufferType*>& recv_buf,
                       size_t count,
                       const alltoall_attr_t& attr = alltoall_attr_t());

    /**
     * Alltoall is a collective communication operation in which each rank
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
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    request_t alltoallv(const void* send_buf,
                        const vector_class<size_t>& send_counts,
                        void* recv_buf,
                        const vector_class<size_t>& recv_counts,
                        datatype dtype,
                        const alltoallv_attr_t& attr = alltoallv_attr_t());

    /**
     * @param send_bufs array of buffers to store send blocks, one buffer per each rank
     * @param send_counts array with number of elements of type @c dtype in send blocks for each rank
     * @param recv_bufs [out] array of buffers to store receive blocks, one buffer per each rank
     * @param recv_counts array with number of elements of type @c dtype in receive blocks from each rank
     * @param dtype datatype of elements in @c send_buf and @c recv_buf
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    request_t alltoallv(const vector_class<void*>& send_bufs,
                        const vector_class<size_t>& send_counts,
                        const vector_class<void*>& recv_bufs,
                        const vector_class<size_t>& recv_counts,
                        datatype dtype,
                        const alltoallv_attr_t& attr = alltoallv_attr_t());

    /**
     * Type safety version:
     * @param send_buf the buffer with elements of @c BufferType that stores local blocks to be sent to each rank
     * @param send_counts array with number of elements of type @c BufferType in send blocks for each rank
     * @param recv_buf [out] the buffer to store received result, should be large enough to hold blocks from all ranks
     * @param recv_counts array with number of elements of type @c BufferType in receive blocks from each rank
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    template <class BufferType,
              class = typename std::enable_if<ccl::is_native_type_supported<BufferType>()>::type>
    request_t alltoallv(const BufferType* send_buf,
                        const vector_class<size_t>& send_counts,
                        BufferType* recv_buf,
                        const vector_class<size_t>& recv_counts,
                        const alltoallv_attr_t& attr = alltoallv_attr_t());

    /**
     * Type safety version:
     * @param send_bufs array of buffers to store send blocks, one buffer per each rank
     * @param send_counts array with number of elements of type @c BufferType in send blocks for each rank
     * @param recv_bufs [out] array of buffers to store receive blocks, one buffer per each rank
     * @param recv_counts array with number of elements of type @c BufferType in receive blocks from each rank
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    template <class BufferType,
              class = typename std::enable_if<ccl::is_native_type_supported<BufferType>()>::type>
    request_t alltoallv(const vector_class<BufferType*>& send_bufs,
                        const vector_class<size_t>& send_counts,
                        const vector_class<BufferType*>& recv_bufs,
                        const vector_class<size_t>& recv_counts,
                        const alltoallv_attr_t& attr = alltoallv_attr_t());

    /**
     * Barrier synchronization across all ranks of communicator.
     * Completes after all ranks in the communicator have called it.
     *
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    request_t barrier(const barrier_attr_t& attr = barrier_attr_t());

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
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    request_t bcast(void* buf,
                    size_t count,
                    datatype dtype,
                    size_t root,
                    const bcast_attr_t& attr = bcast_attr_t());

    /**
     * Type safety version:
     * @param buf [in,out] the buffer with @c count elements of @c BufferType
     * serves as send buffer for root and as receive buffer for other ranks
     * @param count number of elements of type @c BufferType in @c buf
     * @param root the rank that broadcasts @c buf
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    template <class BufferType,
              class = typename std::enable_if<ccl::is_native_type_supported<BufferType>()>::type>
    request_t bcast(BufferType* buf,
                    size_t count,
                    size_t root,
                    const bcast_attr_t& attr = bcast_attr_t());

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
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    request_t reduce(const void* send_buf,
                     void* recv_buf,
                     size_t count,
                     datatype dtype,
                     reduction reduction,
                     size_t root,
                     const reduce_attr_t& attr = reduce_attr_t());

    /**
     * Type safety version:
     * @param send_buf the buffer with @c count elements of @c BufferType that stores local data to be reduced
     * @param recv_buf [out] the buffer to store reduced result, must have the same dimension as @c send_buf.
     * Used by the @c root rank only, ignored by other ranks.
     * @param count number of elements of type @c BufferType in @c send_buf and @c recv_buf
     * @param reduction type of reduction operation to be applied
     * @param root the rank that gets the result of reduction
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
                     const reduce_attr_t& attr = reduce_attr_t());

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
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    request_t reduce_scatter(const void* send_buf,
                             void* recv_buf,
                             size_t recv_count,
                             datatype dtype,
                             reduction reduction,
                             const reduce_scatter_attr_t& attr = reduce_scatter_attr_t());

    /**
     * Type safety version:
     * @param send_buf the buffer with @c comm_size * @c count elements of @c BufferType that stores local data to be reduced
     * @param recv_buf [out] the buffer to store result block containing @c recv_count elements of type @c BufferType
     * @param recv_count number of elements of type @c BufferType in receive block
     * @param reduction type of reduction operation to be applied
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    template <class BufferType,
              class = typename std::enable_if<ccl::is_native_type_supported<BufferType>()>::type>
    request_t reduce_scatter(const BufferType* send_buf,
                             BufferType* recv_buf,
                             size_t recv_count,
                             reduction reduction,
                             const reduce_scatter_attr_t& attr = reduce_scatter_attr_t());

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
                               const sparse_allreduce_attr_t& attr = sparse_allreduce_attr_t());

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
                               const sparse_allreduce_attr_t& attr = sparse_allreduce_attr_t());

private:
    friend class environment;

    explicit communicator(shared_ptr_class<communicator_interface> impl);

    /**
     * Holds implementation specific details
     */
    shared_ptr_class<communicator_interface> pimpl;
};

/**
 * A event object is an abstraction over native event object
 * to be used for operation status tracking.
 */
class event : public non_copyable<event>,
              public non_movable<event>,
              public pointer_on_impl<event, ccl_event> {
public:
    using native_handle_t = typename unified_event_type::native_reference_t;
    using impl_value_t = typename pointer_on_impl<event, ccl_event>::impl_value_t;

#ifdef DEVICE_COMM_SUPPORT
    event(native_event_type& native_event);
    native_event_type get() const;
#endif

    event();

private:
    friend class communicator;
    friend class environment;
    event(impl_value_t&& impl);
};

#ifdef DEVICE_COMM_SUPPORT

class device_communicator final
        : public pointer_on_impl<device_communicator, communicator_interface> {
public:
    using native_handle_t = typename unified_device_type::native_reference_t;
    using impl_value_t =
        typename pointer_on_impl<device_communicator, communicator_interface>::impl_value_t;
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
     * Type allows to get underlying device type,
     * which was used as communicator construction argument
     */
    //using native_device_reference_t = native_handle_t;

    /**
     * Retrieves underlying device, which was used as communicator construction argument
     */
    native_device_type get_device() const;

    /**
     * Retrieves underlying context, which was used as communicator construction argument
     */
    native_context_type get_context() const;

    device_communicator split(device_comm_split_attr_t attr);

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
     * @param stream stream associated with the operation
     * @param deps optional vector of events that the operation should depend on
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    request_t allgatherv(const void* send_buf,
                         size_t send_count,
                         void* recv_buf,
                         const vector_class<size_t>& recv_counts,
                         datatype dtype,
                         const stream_t& stream,
                         const vector_class<event_t>& deps = {},
                         const allgatherv_attr_t& attr = allgatherv_attr_t());

    /**
     * @param send_buf the buffer with @c send_count elements of @c dtype that stores local data to be gathered
     * @param send_count number of elements of type @c dtype in @c send_buf
     * @param recv_bufs [out] array of buffers to store gathered result, one buffer per each rank
     * @param recv_counts array with number of elements of type @c dtype to be received from each rank
     * @param dtype datatype of elements in @c send_buf and @c recv_buf
     * @param stream stream associated with the operation
     * @param deps optional vector of events that the operation should depend on
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    request_t allgatherv(const void* send_buf,
                         size_t send_count,
                         const vector_class<void*>& recv_bufs,
                         const vector_class<size_t>& recv_counts,
                         datatype dtype,
                         const stream_t& stream,
                         const vector_class<event_t>& deps = {},
                         const allgatherv_attr_t& attr = allgatherv_attr_t());
    /**
     * Type safety version:
     * @param send_buf the buffer with @c send_count elements of @c BufferType that stores local data to be gathered
     * @param send_count number of elements of type @c BufferType in @c send_buf
     * @param recv_buf [out] the buffer to store gathered result, should be large enough to hold values from all ranks
     * @param recv_counts array with number of elements of type @c BufferType to be received from each rank
     * @param stream stream associated with the operation
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
                         const stream_t& stream,
                         const vector_class<event_t>& deps = {},
                         const allgatherv_attr_t& attr = allgatherv_attr_t());

    /**
     * Type safety version:
     * @param send_buf the buffer with @c send_count elements of @c BufferType that stores local data to be gathered
     * @param send_count number of elements of type @c BufferType in @c send_buf
     * @param recv_bufs [out] array of buffers to store gathered result, one buffer per each rank
     * @param recv_counts array with number of elements of type @c BufferType to be received from each rank
     * @param stream stream associated with the operation
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
                         const stream_t& stream,
                         const vector_class<event_t>& deps = {},
                         const allgatherv_attr_t& attr = allgatherv_attr_t());

    /**
     * Type safety version:
     * @param send_buf the buffer with @c send_count elements of @c dtype that stores local data to be gathered
     * @param send_count number of elements of type @c dtype in @c send_buf
     * @param recv_buf [out] the buffer to store gathered result, should be large enough to hold values from all ranks
     * @param recv_counts array with number of elements of type @c dtype to be received from each rank
     * @param stream stream associated with the operation
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
                         const stream_t& stream,
                         const vector_class<event_t>& deps = {},
                         const allgatherv_attr_t& attr = allgatherv_attr_t());

    /**
     * Type safety version:
     * @param send_buf the buffer with @c send_count elements of @c dtype that stores local data to be gathered
     * @param send_count number of elements of type @c dtype in @c send_buf
     * @param recv_bufs [out] array of buffers to store gathered result, one buffer per each rank
     * @param recv_counts array with number of elements of type @c dtype to be received from each rank
     * @param stream stream associated with the operation
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
                         const stream_t& stream,
                         const vector_class<event_t>& deps = {},
                         const allgatherv_attr_t& attr = allgatherv_attr_t());

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
     * @param stream stream associated with the operation
     * @param deps optional vector of events that the operation should depend on
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    request_t allreduce(const void* send_buf,
                        void* recv_buf,
                        size_t count,
                        datatype dtype,
                        reduction reduction,
                        const stream_t& stream,
                        const vector_class<event_t>& deps = {},
                        const allreduce_attr_t& attr = allreduce_attr_t());

    /**
     * Type safety version:
     * @param send_buf the buffer with @c count elements of @c BufferType that stores local data to be reduced
     * @param recv_buf [out] the buffer to store reduced result, must have the same dimension as @c send_buf
     * @param count number of elements of type @c BufferType in @c send_buf and @c recv_buf
     * @param reduction type of reduction operation to be applied
     * @param stream stream associated with the operation
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
                        const stream_t& stream,
                        const vector_class<event_t>& deps = {},
                        const allreduce_attr_t& attr = allreduce_attr_t());

    /**
     * Type safety version:
     * @param send_buf the buffer with @c count elements of @c dtype that stores local data to be reduced
     * @param recv_buf [out] the buffer to store reduced result, must have the same dimension as @c send_buf
     * @param count number of elements of type @c dtype in @c send_buf and @c recv_buf
     * @param reduction type of reduction operation to be applied
     * @param stream stream associated with the operation
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
                        const stream_t& stream,
                        const vector_class<event_t>& deps = {},
                        const allreduce_attr_t& attr = allreduce_attr_t());

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
     * @param stream stream associated with the operation
     * @param deps optional vector of events that the operation should depend on
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    request_t alltoall(const void* send_buf,
                       void* recv_buf,
                       size_t count,
                       datatype dtype,
                       const stream_t& stream,
                       const vector_class<event_t>& deps = {},
                       const alltoall_attr_t& attr = alltoall_attr_t());

    /**
     * @param send_bufs array of buffers with local data to be sent, one buffer per each rank
     * @param recv_bufs [out] array of buffers to store received result, one buffer per each rank
     * @param count number of elements of type @c dtype to be send to or to received from each rank
     * @param dtype datatype of elements in @c send_buf and @c recv_buf
     * @param stream stream associated with the operation
     * @param deps optional vector of events that the operation should depend on
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    request_t alltoall(const vector_class<void*>& send_buf,
                       const vector_class<void*>& recv_buf,
                       size_t count,
                       datatype dtype,
                       const stream_t& stream,
                       const vector_class<event_t>& deps = {},
                       const alltoall_attr_t& attr = alltoall_attr_t());

    /**
     * Type safety version:
     * @param send_buf the buffer with @c count elements of @c BufferType that stores local data to be sent
     * @param recv_buf [out] the buffer to store received result, should be large enough
     * to hold values from all ranks, i.e. at least @c comm_size * @c count
     * @param count number of elements to be send to or to received from each rank
     * @param stream stream associated with the operation
     * @param deps optional vector of events that the operation should depend on
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    template <class BufferType,
              class = typename std::enable_if<ccl::is_native_type_supported<BufferType>()>::type>
    request_t alltoall(const BufferType* send_buf,
                       BufferType* recv_buf,
                       size_t count,
                       const stream_t& stream,
                       const vector_class<event_t>& deps = {},
                       const alltoall_attr_t& attr = alltoall_attr_t());

    /**
     * Type safety version:
     * @param send_bufs array of buffers with local data to be sent, one buffer per each rank
     * @param recv_bufs [out] array of buffers to store received result, one buffer per each rank
     * @param count number of elements to be send to or to received from each rank
     * @param stream stream associated with the operation
     * @param deps optional vector of events that the operation should depend on
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    template <class BufferType,
              class = typename std::enable_if<ccl::is_native_type_supported<BufferType>()>::type>
    request_t alltoall(const vector_class<BufferType*>& send_buf,
                       const vector_class<BufferType*>& recv_buf,
                       size_t count,
                       const stream_t& stream,
                       const vector_class<event_t>& deps = {},
                       const alltoall_attr_t& attr = alltoall_attr_t());

    /**
     * Type safety version:
     * @param send_buf the buffer with @c count elements of @c dtype that stores local data to be sent
     * @param recv_buf [out] the buffer to store received result, should be large enough
     * to hold values from all ranks, i.e. at least @c comm_size * @c count
     * @param count number of elements to be send to or to received from each rank
     * @param stream stream associated with the operation
     * @param deps optional vector of events that the operation should depend on
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    template <class BufferObjectType,
              class = typename std::enable_if<ccl::is_class_supported<BufferObjectType>()>::type>
    request_t alltoall(const BufferObjectType& send_buf,
                       BufferObjectType& recv_buf,
                       size_t count,
                       const stream_t& stream,
                       const vector_class<event_t>& deps = {},
                       const alltoall_attr_t& attr = alltoall_attr_t());

    /**
     * Type safety version:
     * @param send_bufs array of buffers with local data to be sent, one buffer per each rank
     * @param recv_bufs [out] array of buffers to store received result, one buffer per each rank
     * @param count number of elements to be send to or to received from each rank
     * @param stream stream associated with the operation
     * @param deps optional vector of events that the operation should depend on
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    template <class BufferObjectType,
              class = typename std::enable_if<ccl::is_class_supported<BufferObjectType>()>::type>
    request_t alltoall(const vector_class<BufferObjectType&>& send_buf,
                       const vector_class<BufferObjectType&>& recv_buf,
                       size_t count,
                       const stream_t& stream,
                       const vector_class<event_t>& deps = {},
                       const alltoall_attr_t& attr = alltoall_attr_t());

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
     * @param stream stream associated with the operation
     * @param deps optional vector of events that the operation should depend on
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    request_t alltoallv(const void* send_buf,
                        const vector_class<size_t>& send_counts,
                        void* recv_buf,
                        const vector_class<size_t>& recv_counts,
                        datatype dtype,
                        const stream_t& stream,
                        const vector_class<event_t>& deps = {},
                        const alltoallv_attr_t& attr = alltoallv_attr_t());

    /**
     * @param send_bufs array of buffers to store send blocks, one buffer per each rank
     * @param send_counts array with number of elements of type @c dtype in send blocks for each rank
     * @param recv_bufs [out] array of buffers to store receive blocks, one buffer per each rank
     * @param recv_counts array with number of elements of type @c dtype in receive blocks from each rank
     * @param dtype datatype of elements in @c send_buf and @c recv_buf
     * @param stream stream associated with the operation
     * @param deps optional vector of events that the operation should depend on
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    request_t alltoallv(const vector_class<void*>& send_bufs,
                        const vector_class<size_t>& send_counts,
                        const vector_class<void*>& recv_bufs,
                        const vector_class<size_t>& recv_counts,
                        datatype dtype,
                        const stream_t& stream,
                        const vector_class<event_t>& deps = {},
                        const alltoallv_attr_t& attr = alltoallv_attr_t());

    /**
     * Type safety version:
     * @param send_buf the buffer with elements of @c BufferType that stores local blocks to be sent to each rank
     * @param send_counts array with number of elements of type @c BufferType in send blocks for each rank
     * @param recv_buf [out] the buffer to store received result, should be large enough to hold blocks from all ranks
     * @param recv_counts array with number of elements of type @c BufferType in receive blocks from each rank
     * @param stream stream associated with the operation
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
                        const stream_t& stream,
                        const vector_class<event_t>& deps = {},
                        const alltoallv_attr_t& attr = alltoallv_attr_t());

    /**
     * Type safety version:
     * @param send_bufs array of buffers to store send blocks, one buffer per each rank
     * @param send_counts array with number of elements of type @c BufferType in send blocks for each rank
     * @param recv_bufs [out] array of buffers to store receive blocks, one buffer per each rank
     * @param recv_counts array with number of elements of type @c BufferType in receive blocks from each rank
     * @param stream stream associated with the operation
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
                        const stream_t& stream,
                        const vector_class<event_t>& deps = {},
                        const alltoallv_attr_t& attr = alltoallv_attr_t());

    /**
     * Type safety version:
     * @param send_buf the buffer with elements of @c dtype that stores local blocks to be sent to each rank
     * @param send_counts array with number of elements of type @c dtype in send blocks for each rank
     * @param recv_buf [out] the buffer to store received result, should be large enough to hold blocks from all ranks
     * @param recv_counts array with number of elements of type @c dtype in receive blocks from each rank
     * @param stream stream associated with the operation
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
                        const stream_t& stream,
                        const vector_class<event_t>& deps = {},
                        const alltoallv_attr_t& attr = alltoallv_attr_t());

    /**
     * Type safety version:
     * @param send_bufs array of buffers to store send blocks, one buffer per each rank
     * @param send_counts array with number of elements of type @c dtype in send blocks for each rank
     * @param recv_bufs [out] array of buffers to store receive blocks, one buffer per each rank
     * @param recv_counts array with number of elements of type @c dtype in receive blocks from each rank
     * @param dtype datatype of elements in @c send_buf and @c recv_buf
     * @param stream stream associated with the operation
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
                        const stream_t& stream,
                        const vector_class<event_t>& deps = {},
                        const alltoallv_attr_t& attr = alltoallv_attr_t());

    /**
     * Barrier synchronization across all ranks of communicator.
     * Completes after all ranks in the communicator have called it.
     *
     * @param stream stream associated with the operation
     * @param deps optional vector of events that the operation should depend on
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    request_t barrier(const stream_t& stream,
                      const vector_class<event_t>& deps = {},
                      const barrier_attr_t& attr = barrier_attr_t());

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
     * @param stream stream associated with the operation
     * @param deps optional vector of events that the operation should depend on
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    request_t bcast(void* buf,
                    size_t count,
                    datatype dtype,
                    size_t root,
                    const stream_t& stream,
                    const vector_class<event_t>& deps = {},
                    const bcast_attr_t& attr = bcast_attr_t());

    /**
     * Type safety version:
     * @param buf [in,out] the buffer with @c count elements of @c BufferType
     * serves as send buffer for root and as receive buffer for other ranks
     * @param count number of elements of type @c BufferType in @c buf
     * @param root the rank that broadcasts @c buf
     * @param stream stream associated with the operation
     * @param deps optional vector of events that the operation should depend on
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    template <class BufferType,
              class = typename std::enable_if<ccl::is_native_type_supported<BufferType>()>::type>
    request_t bcast(BufferType* buf,
                    size_t count,
                    size_t root,
                    const stream_t& stream,
                    const vector_class<event_t>& deps = {},
                    const bcast_attr_t& attr = bcast_attr_t());

    /**
     * Type safety version:
     * @param buf [in,out] the buffer with @c count elements of @c dtype
     * serves as send buffer for root and as receive buffer for other ranks
     * @param count number of elements of type @c dtype in @c buf
     * @param root the rank that broadcasts @c buf
     * @param stream stream associated with the operation
     * @param deps optional vector of events that the operation should depend on
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    template <class BufferObjectType,
              class = typename std::enable_if<ccl::is_class_supported<BufferObjectType>()>::type>
    request_t bcast(BufferObjectType& buf,
                    size_t count,
                    size_t root,
                    const stream_t& stream,
                    const vector_class<event_t>& deps = {},
                    const bcast_attr_t& attr = bcast_attr_t());

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
     * @param stream stream associated with the operation
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
                     const stream_t& stream,
                     const vector_class<event_t>& deps = {},
                     const reduce_attr_t& attr = reduce_attr_t());

    /**
     * Type safety version:
     * @param send_buf the buffer with @c count elements of @c BufferType that stores local data to be reduced
     * @param recv_buf [out] the buffer to store reduced result, must have the same dimension as @c send_buf.
     * Used by the @c root rank only, ignored by other ranks.
     * @param count number of elements of type @c BufferType in @c send_buf and @c recv_buf
     * @param reduction type of reduction operation to be applied
     * @param root the rank that gets the result of reduction
     * @param stream stream associated with the operation
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
                     const stream_t& stream,
                     const vector_class<event_t>& deps = {},
                     const reduce_attr_t& attr = reduce_attr_t());

    /**
     * Type safety version:
     * @param send_buf the buffer with @c count elements of @c dtype that stores local data to be reduced
     * @param recv_buf [out] the buffer to store reduced result, must have the same dimension as @c send_buf.
     * Used by the @c root rank only, ignored by other ranks.
     * @param count number of elements of type @c dtype in @c send_buf and @c recv_buf
     * @param dtype datatype of elements in @c send_buf and @c recv_buf
     * @param reduction type of reduction operation to be applied
     * @param stream stream associated with the operation
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
                     const stream_t& stream,
                     const vector_class<event_t>& deps = {},
                     const reduce_attr_t& attr = reduce_attr_t());

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
     * @param stream stream associated with the operation
     * @param deps optional vector of events that the operation should depend on
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    request_t reduce_scatter(const void* send_buf,
                             void* recv_buf,
                             size_t recv_count,
                             datatype dtype,
                             reduction reduction,
                             const stream_t& stream,
                             const vector_class<event_t>& deps = {},
                             const reduce_scatter_attr_t& attr = reduce_scatter_attr_t());

    /**
     * Type safety version:
     * @param send_buf the buffer with @c comm_size * @c count elements of @c BufferType that stores local data to be reduced
     * @param recv_buf [out] the buffer to store result block containing @c recv_count elements of type @c BufferType
     * @param recv_count number of elements of type @c BufferType in receive block
     * @param reduction type of reduction operation to be applied
     * @param stream stream associated with the operation
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
                             const stream_t& stream,
                             const vector_class<event_t>& deps = {},
                             const reduce_scatter_attr_t& attr = reduce_scatter_attr_t());

    /**
     * Type safety version:
     * @param send_buf the buffer with @c comm_size * @c count elements of @c dtype that stores local data to be reduced
     * @param recv_buf [out] the buffer to store result block containing @c recv_count elements of type @c dtype
     * @param recv_count number of elements of type @c dtype in receive block
     * @param reduction type of reduction operation to be applied
     * @param stream stream associated with the operation
     * @param deps optional vector of events that the operation should depend on
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    // template <class BufferObjectType,
    //           class = typename std::enable_if<ccl::is_class_supported<BufferObjectType>()>::type>

    /* TODO: apply this in other places */
    template <class BufferObjectType>
    typename std::enable_if<ccl::is_class_supported<BufferObjectType>(), request_t>::type
    reduce_scatter(const BufferObjectType& send_buf,
                   BufferObjectType& recv_buf,
                   size_t recv_count,
                   reduction reduction,
                   const stream_t& stream,
                   const vector_class<event_t>& deps = {},
                   const reduce_scatter_attr_t& attr = reduce_scatter_attr_t());

private:
    friend class environment;
    friend class device_comm_group;

    explicit device_communicator(impl_value_t impl);
};
#endif /* DEVICE_COMM_SUPPORT */

} // namespace ccl

#ifdef DEVICE_COMM_SUPPORT
#include "ccl_gpu_modules.h"
#include "gpu_communicator.hpp"
#endif /* DEVICE_COMM_SUPPORT */
