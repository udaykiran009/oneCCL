#pragma once

#ifndef CCL_PRODUCT_FULL
#error "Do not include this file directly. Please include 'ccl.hpp'"
#endif

namespace ccl {

void init();

/******************** ENVIRONMENT ********************/

/**
 * Retrieves the library version
 */
library_version get_library_version();

/**
 * Creates @attr which used to register custom datatype
 */
template <class... attr_value_pair_t>
datatype_attr CCL_API create_datatype_attr(attr_value_pair_t&&... avps) {
    return environment::instance().create_datatype_attr(std::forward<attr_value_pair_t>(avps)...);
}

/**
 * Registers custom datatype to be used in communication operations
 * @param attr datatype attributes
 * @return datatype handle
 */
datatype register_datatype(const datatype_attr& attr);

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
size_t get_datatype_size(datatype dtype);

/******************** KVS ********************/

/**
 * Creates a main key-value store.
 * It's address should be distributed using out of band communication mechanism
 * and be used to create key-value stores on other ranks.
 * @return kvs object
 */
shared_ptr_class<kvs> create_main_kvs();

/**
 * Creates a new key-value store from main kvs address
 * @param addr address of main kvs
 * @return kvs object
 */
shared_ptr_class<kvs> create_kvs(const kvs::address_type& addr);

template <class coll_attribute_type, class... attr_value_pair_t>
coll_attribute_type CCL_API create_operation_attr(attr_value_pair_t&&... avps) {
    return environment::instance().create_operation_attr<coll_attribute_type>(
        std::forward<attr_value_pair_t>(avps)...);
}

#ifdef CCL_ENABLE_SYCL
communicator create_single_device_communicator(size_t comm_size,
                                                      size_t rank,
                                                      const cl::sycl::device& device,
                                                      shared_ptr_class<kvs_interface> kvs);
#endif

template <class... attr_value_pair_t>
comm_split_attr create_device_comm_split_attr(attr_value_pair_t&&... avps) {
    return environment::instance().create_device_comm_split_attr(
    std::forward<attr_value_pair_t>(avps)...);
}


/**
 * Creates a new device from @native_device_type
 * @param native_device the existing handle of device
 * @return device object
 */
device create_device();

template <class native_device_type,
          class = typename std::enable_if<is_device_supported<native_device_type>()>::type>
device create_device(native_device_type&& native_device) {
    return environment::instance().create_device(std::forward<native_device_type>(native_device));
}

template <class... attr_value_pair_t>
device create_device_from_attr(typename unified_device_type::ccl_native_t dev,
                               attr_value_pair_t&&... avps) {
    return environment::instance().create_device_from_attr(
        dev, std::forward<attr_value_pair_t>(avps)...);
}

/**
 * Creates a new context from @native_device_contex_type
 * @param native_device_context the existing handle of context
 * @return context object
 */
context create_context();

template <class native_device_context_type,
          class = typename std::enable_if<is_context_supported<native_device_context_type>()>::type>
context create_context(native_device_context_type&& native_device_context) {
    return environment::instance().create_context(std::forward<native_device_context_type>(native_device_context));
}

template <class... attr_value_pair_t>
context create_context_from_attr(typename unified_device_context_type::ccl_native_t ctx,
                               attr_value_pair_t&&... avps) {
    return environment::instance().create_context_from_attr(
        ctx, std::forward<attr_value_pair_t>(avps)...);
}

/**
 * Creates a new event from @native_event_type
 * @param native_event the existing handle of event
 * @return event object
 */
template <class event_type,
          class = typename std::enable_if<is_event_supported<event_type>()>::type>
event create_event(event_type& native_event) {
    // TODO not implemented
    throw ccl::exception(std::string(__PRETTY_FUNCTION__) + " - is not implemented");
    return {};
}

template <class event_handle_type,
          class = typename std::enable_if<is_event_supported<event_handle_type>()>::type>
event create_event(event_handle_type native_event_handle,
                   typename unified_device_context_type::ccl_native_t context) {
    // TODO not implemented
    throw ccl::exception(std::string(__PRETTY_FUNCTION__) + " - is not implemented");
    return {};
}

template <class event_type, class... attr_value_pair_t>
event create_event_from_attr(event_type& native_event_handle,
                             typename unified_device_context_type::ccl_native_t context,
                             attr_value_pair_t&&... avps) {
    // TODO not implemented
    throw ccl::exception(std::string(__PRETTY_FUNCTION__) + " - is not implemented");
    return {};
}

/**
 * Creates a new stream from @native_stream_type
 * @param native_stream the existing handle of stream
 * @return stream object
 */
stream create_stream();

template <class native_stream_type,
          class = typename std::enable_if<is_stream_supported<native_stream_type>()>::type>
stream create_stream(native_stream_type& native_stream) {
    return environment::instance().create_stream(native_stream);
}

template <class native_stream_type, class native_context_type,
          class = typename std::enable_if<is_stream_supported<native_stream_type>()>::type>
stream create_stream(native_stream_type& native_stream, native_context_type& native_ctx) {
    return environment::instance().create_stream(native_stream, native_ctx);
}

template <class... attr_value_pair_t>
stream create_stream_from_attr(typename unified_device_type::ccl_native_t device,
                               attr_value_pair_t&&... avps) {
    return environment::instance().create_stream_from_attr(
        device, std::forward<attr_value_pair_t>(avps)...);
}

template <class... attr_value_pair_t>
stream create_stream_from_attr(typename unified_device_type::ccl_native_t device,
                               typename unified_device_context_type::ccl_native_t context,
                               attr_value_pair_t&&... avps) {
    return environment::instance().create_stream_from_attr(
        device, context, std::forward<attr_value_pair_t>(avps)...);
}

namespace preview {

/**
 * Splits device communicators according to attributes.
 * @param attrs split attributes for local communicators
 * @return vector of device communicators
 */
vector_class<communicator> split_device_communicators(
    const vector_class<pair_class<communicator, comm_split_attr>>& attrs);


/**
 * Creates a new host communicator with externally provided size, rank and kvs.
 * Implementation is platform specific and non portable.
 * @return host communicator
 */
communicator create_communicator();

/**
 * Creates a new host communicator with user supplied size and kvs.
 * Rank will be assigned automatically.
 * @param size user-supplied total number of ranks
 * @param kvs key-value store for ranks wire-up
 * @return host communicator
 */
communicator create_communicator(size_t size, shared_ptr_class<kvs_interface> kvs);

} // namespace preview

/**
 * Creates a new host communicator with user supplied size, rank and kvs.
 * @param size user-supplied total number of ranks
 * @param rank user-supplied rank
 * @param kvs key-value store for ranks wire-up
 * @return host communicator
 */
communicator create_communicator(size_t size,
                                 size_t rank,
                                 shared_ptr_class<kvs_interface> kvs);

// #if defined(MULTI_GPU_SUPPORT) || defined(CCL_ENABLE_SYCL)

/**
 * Creates a new device communicators with user supplied size, device indices and kvs.
 * Ranks will be assigned automatically.
 * @param comm_size user-supplied total number of ranks
 * @param local_devices user-supplied device objects for local ranks
 * @param context context containing the devices
 * @param kvs key-value store for ranks wire-up
 * @return vector of device communicators
 */
template <class DeviceType, class ContextType>
vector_class<communicator> create_device_communicators(
    size_t comm_size,
    const vector_class<DeviceType>& local_devices,
    ContextType& context,
    shared_ptr_class<kvs_interface> kvs) {
    return environment::instance().create_device_communicators(
        comm_size, local_devices, context, kvs);
}

/**
 * Creates a new device communicators with user supplied size, ranks, device indices and kvs.
 * @param comm_size user-supplied total number of ranks
 * @param local_rank_device_map user-supplied mapping of local ranks on devices
 * @param context context containing the devices
 * @param kvs key-value store for ranks wire-up
 * @return vector of device communicators
 */
template <class DeviceType, class ContextType>
vector_class<communicator> create_device_communicators(
    size_t comm_size,
    const vector_class<pair_class<rank_t, DeviceType>>& local_rank_device_map,
    ContextType& context,
    shared_ptr_class<kvs_interface> kvs) {
    return environment::instance().create_device_communicators(
        comm_size, local_rank_device_map, context, kvs);
}

template <class DeviceType, class ContextType>
vector_class<communicator> create_device_communicators(
    size_t comm_size,
    const map_class<rank_t, DeviceType>& local_rank_device_map,
    ContextType& context,
    shared_ptr_class<kvs_interface> kvs) {
    return environment::instance().create_device_communicators(
        comm_size, local_rank_device_map, context, kvs);
}

/******************** DEVICE COMMUNICATOR ********************/

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
 * @param comm the communicator for which the operation will be performed
 * @param op_stream op_stream associated with the operation
 * @param attr optional attributes to customize operation
 * @param deps optional vector of events that the operation should depend on
 * @return @ref ccl::event object to track the progress of the operation
 */
event allgatherv(const void* send_buf,
                   size_t send_count,
                   void* recv_buf,
                   const vector_class<size_t>& recv_counts,
                   datatype dtype,
                   const communicator& comm,
                   stream& op_stream = default_stream,
                   const allgatherv_attr& attr = default_allgatherv_attr,
                   const vector_class<event>& deps = {});

/**
 * @param send_buf the buffer with @c send_count elements of @c dtype that stores local data to be gathered
 * @param send_count number of elements of type @c dtype in @c send_buf
 * @param recv_bufs [out] array of buffers to store gathered result, one buffer per each rank
 * @param recv_counts array with number of elements of type @c dtype to be received from each rank
 * @param dtype datatype of elements in @c send_buf and @c recv_buf
 * @param comm the communicator for which the operation will be performed
 * @param op_stream op_stream associated with the operation
 * @param attr optional attributes to customize operation
 * @param deps optional vector of events that the operation should depend on
 * @return @ref ccl::event object to track the progress of the operation
 */
event allgatherv(const void* send_buf,
                   size_t send_count,
                   const vector_class<void*>& recv_bufs,
                   const vector_class<size_t>& recv_counts,
                   datatype dtype,
                   const communicator& comm,
                   stream& op_stream = default_stream,
                   const allgatherv_attr& attr = default_allgatherv_attr,
                   const vector_class<event>& deps = {});

/**
 * Type safety version:
 * @param send_buf the buffer with @c send_count elements of @c BufferType that stores local data to be gathered
 * @param send_count number of elements of type @c BufferType in @c send_buf
 * @param recv_buf [out] the buffer to store gathered result, should be large enough to hold values from all ranks
 * @param recv_counts array with number of elements of type @c BufferType to be received from each rank
 * @param comm the communicator for which the operation will be performed
 * @param op_stream op_stream associated with the operation
 * @param attr optional attributes to customize operation
 * @param deps optional vector of events that the operation should depend on
 * @return @ref ccl::event object to track the progress of the operation
 */
template <class BufferType,
          class = typename std::enable_if<is_native_type_supported<BufferType>(), event>::type>
event allgatherv(const BufferType* send_buf,
                   size_t send_count,
                   BufferType* recv_buf,
                   const vector_class<size_t>& recv_counts,
                   const communicator& comm,
                   stream& op_stream = default_stream,
                   const allgatherv_attr& attr = default_allgatherv_attr,
                   const vector_class<event>& deps = {});

/**
 * Type safety version:
 * @param send_buf the buffer with @c send_count elements of @c BufferType that stores local data to be gathered
 * @param send_count number of elements of type @c BufferType in @c send_buf
 * @param recv_bufs [out] array of buffers to store gathered result, one buffer per each rank
 * @param recv_counts array with number of elements of type @c BufferType to be received from each rank
 * @param comm the communicator for which the operation will be performed
 * @param op_stream op_stream associated with the operation
 * @param attr optional attributes to customize operation
 * @param deps optional vector of events that the operation should depend on
 * @return @ref ccl::event object to track the progress of the operation
 */
template <class BufferType,
          class = typename std::enable_if<is_native_type_supported<BufferType>(), event>::type>
event allgatherv(const BufferType* send_buf,
                   size_t send_count,
                   vector_class<BufferType*>& recv_bufs,
                   const vector_class<size_t>& recv_counts,
                   const communicator& comm,
                   stream& op_stream = default_stream,
                   const allgatherv_attr& attr = default_allgatherv_attr,
                   const vector_class<event>& deps = {});

/**
 * Type safety version:
 * @param send_buf the buffer with @c send_count elements of @c dtype that stores local data to be gathered
 * @param send_count number of elements of type @c dtype in @c send_buf
 * @param recv_buf [out] the buffer to store gathered result, should be large enough to hold values from all ranks
 * @param recv_counts array with number of elements of type @c dtype to be received from each rank
 * @param comm the communicator for which the operation will be performed
 * @param op_stream op_stream associated with the operation
 * @param attr optional attributes to customize operation
 * @param deps optional vector of events that the operation should depend on
 * @return @ref ccl::event object to track the progress of the operation
 */
template <class BufferObjectType,
          class = typename std::enable_if<is_class_supported<BufferObjectType>(), event>::type>
event allgatherv(const BufferObjectType& send_buf,
                   size_t send_count,
                   BufferObjectType& recv_buf,
                   const vector_class<size_t>& recv_counts,
                   const communicator& comm,
                   stream& op_stream = default_stream,
                   const allgatherv_attr& attr = default_allgatherv_attr,
                   const vector_class<event>& deps = {});

/**
 * Type safety version:
 * @param send_buf the buffer with @c send_count elements of @c dtype that stores local data to be gathered
 * @param send_count number of elements of type @c dtype in @c send_buf
 * @param recv_bufs [out] array of buffers to store gathered result, one buffer per each rank
 * @param recv_counts array with number of elements of type @c dtype to be received from each rank
 * @param comm the communicator for which the operation will be performed
 * @param op_stream op_stream associated with the operation
 * @param attr optional attributes to customize operation
 * @param deps optional vector of events that the operation should depend on
 * @return @ref ccl::event object to track the progress of the operation
 */
template <class BufferObjectType,
          class = typename std::enable_if<is_class_supported<BufferObjectType>(), event>::type>
event allgatherv(const BufferObjectType& send_buf,
                   size_t send_count,
                   vector_class<reference_wrapper_class<BufferObjectType>>& recv_bufs,
                   const vector_class<size_t>& recv_counts,
                   const communicator& comm,
                   stream& op_stream = default_stream,
                   const allgatherv_attr& attr = default_allgatherv_attr,
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
 * @param rtype type of reduction operation to be applied
 * @param comm the communicator for which the operation will be performed
 * @param op_stream op_stream associated with the operation
 * @param attr optional attributes to customize operation
 * @param deps optional vector of events that the operation should depend on
 * @return @ref ccl::event object to track the progress of the operation
 */
event allreduce(const void* send_buf,
                  void* recv_buf,
                  size_t count,
                  datatype dtype,
                  reduction rtype,
                  const communicator& comm,
                  stream& op_stream = default_stream,
                  const allreduce_attr& attr = default_allreduce_attr,
                  const vector_class<event>& deps = {});

/**
 * Type safety version:
 * @param send_buf the buffer with @c count elements of @c BufferType that stores local data to be reduced
 * @param recv_buf [out] the buffer to store reduced result, must have the same dimension as @c send_buf
 * @param count number of elements of type @c BufferType in @c send_buf and @c recv_buf
 * @param rtype type of reduction operation to be applied
 * @param comm the communicator for which the operation will be performed
 * @param op_stream op_stream associated with the operation
 * @param attr optional attributes to customize operation
 * @param deps optional vector of events that the operation should depend on
 * @return @ref ccl::event object to track the progress of the operation
 */
template <class BufferType,
          class = typename std::enable_if<is_native_type_supported<BufferType>(), event>::type>
event allreduce(const BufferType* send_buf,
                  BufferType* recv_buf,
                  size_t count,
                  reduction rtype,
                  const communicator& comm,
                  stream& op_stream = default_stream,
                  const allreduce_attr& attr = default_allreduce_attr,
                  const vector_class<event>& deps = {});

/**
 * Type safety version:
 * @param send_buf the buffer with @c count elements of @c dtype that stores local data to be reduced
 * @param recv_buf [out] the buffer to store reduced result, must have the same dimension as @c send_buf
 * @param count number of elements of type @c dtype in @c send_buf and @c recv_buf
 * @param rtype type of reduction operation to be applied
 * @param comm the communicator for which the operation will be performed
 * @param op_stream op_stream associated with the operation
 * @param attr optional attributes to customize operation
 * @param deps optional vector of events that the operation should depend on
 * @return @ref ccl::event object to track the progress of the operation
 */
template <class BufferObjectType,
          class = typename std::enable_if<is_class_supported<BufferObjectType>(), event>::type>
event allreduce(const BufferObjectType& send_buf,
                  BufferObjectType& recv_buf,
                  size_t count,
                  reduction rtype,
                  const communicator& comm,
                  stream& op_stream = default_stream,
                  const allreduce_attr& attr = default_allreduce_attr,
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
 * @param comm the communicator for which the operation will be performed
 * @param op_stream op_stream associated with the operation
 * @param attr optional attributes to customize operation
 * @param deps optional vector of events that the operation should depend on
 * @return @ref ccl::event object to track the progress of the operation
 */
event alltoall(const void* send_buf,
                 void* recv_buf,
                 size_t count,
                 datatype dtype,
                 const communicator& comm,
                 stream& op_stream = default_stream,
                 const alltoall_attr& attr = default_alltoall_attr,
                 const vector_class<event>& deps = {});

/**
 * @param send_bufs array of buffers with local data to be sent, one buffer per each rank
 * @param recv_bufs [out] array of buffers to store received result, one buffer per each rank
 * @param count number of elements of type @c dtype to be send to or to received from each rank
 * @param dtype datatype of elements in @c send_buf and @c recv_buf
 * @param comm the communicator for which the operation will be performed
 * @param op_stream op_stream associated with the operation
 * @param attr optional attributes to customize operation
 * @param deps optional vector of events that the operation should depend on
 * @return @ref ccl::event object to track the progress of the operation
 */
event alltoall(const vector_class<void*>& send_buf,
                 const vector_class<void*>& recv_buf,
                 size_t count,
                 datatype dtype,
                 const communicator& comm,
                 stream& op_stream = default_stream,
                 const alltoall_attr& attr = default_alltoall_attr,
                 const vector_class<event>& deps = {});

/**
 * Type safety version:
 * @param send_buf the buffer with @c count elements of @c BufferType that stores local data to be sent
 * @param recv_buf [out] the buffer to store received result, should be large enough
 * to hold values from all ranks, i.e. at least @c comm_size * @c count
 * @param count number of elements to be send to or to received from each rank
 * @param comm the communicator for which the operation will be performed
 * @param op_stream op_stream associated with the operation
 * @param attr optional attributes to customize operation
 * @param deps optional vector of events that the operation should depend on
 * @return @ref ccl::event object to track the progress of the operation
 */
template <class BufferType,
          class = typename std::enable_if<is_native_type_supported<BufferType>(), event>::type>
event alltoall(const BufferType* send_buf,
                 BufferType* recv_buf,
                 size_t count,
                 const communicator& comm,
                 stream& op_stream = default_stream,
                 const alltoall_attr& attr = default_alltoall_attr,
                 const vector_class<event>& deps = {});

/**
 * Type safety version:
 * @param send_bufs array of buffers with local data to be sent, one buffer per each rank
 * @param recv_bufs [out] array of buffers to store received result, one buffer per each rank
 * @param count number of elements to be send to or to received from each rank
 * @param comm the communicator for which the operation will be performed
 * @param op_stream op_stream associated with the operation
 * @param attr optional attributes to customize operation
 * @param deps optional vector of events that the operation should depend on
 * @return @ref ccl::event object to track the progress of the operation
 */
template <class BufferType,
          class = typename std::enable_if<is_native_type_supported<BufferType>(), event>::type>
event alltoall(const vector_class<BufferType*>& send_buf,
                 const vector_class<BufferType*>& recv_buf,
                 size_t count,
                 const communicator& comm,
                 stream& op_stream = default_stream,
                 const alltoall_attr& attr = default_alltoall_attr,
                 const vector_class<event>& deps = {});

/**
 * Type safety version:
 * @param send_buf the buffer with @c count elements of @c dtype that stores local data to be sent
 * @param recv_buf [out] the buffer to store received result, should be large enough
 * to hold values from all ranks, i.e. at least @c comm_size * @c count
 * @param count number of elements to be send to or to received from each rank
 * @param comm the communicator for which the operation will be performed
 * @param op_stream op_stream associated with the operation
 * @param attr optional attributes to customize operation
 * @param deps optional vector of events that the operation should depend on
 * @return @ref ccl::event object to track the progress of the operation
 */
template <class BufferObjectType,
          class = typename std::enable_if<is_class_supported<BufferObjectType>(), event>::type>
event alltoall(const BufferObjectType& send_buf,
                 BufferObjectType& recv_buf,
                 size_t count,
                 const communicator& comm,
                 stream& op_stream = default_stream,
                 const alltoall_attr& attr = default_alltoall_attr,
                 const vector_class<event>& deps = {});

/**
 * Type safety version:
 * @param send_bufs array of buffers with local data to be sent, one buffer per each rank
 * @param recv_bufs [out] array of buffers to store received result, one buffer per each rank
 * @param count number of elements to be send to or to received from each rank
 * @param comm the communicator for which the operation will be performed
 * @param op_stream op_stream associated with the operation
 * @param attr optional attributes to customize operation
 * @param deps optional vector of events that the operation should depend on
 * @return @ref ccl::event object to track the progress of the operation
 */
template <class BufferObjectType,
          class = typename std::enable_if<is_class_supported<BufferObjectType>(), event>::type>
event alltoall(const vector_class<reference_wrapper_class<BufferObjectType>>& send_buf,
                 const vector_class<reference_wrapper_class<BufferObjectType>>& recv_buf,
                 size_t count,
                 const communicator& comm,
                 stream& op_stream = default_stream,
                 const alltoall_attr& attr = default_alltoall_attr,
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
 * @param comm the communicator for which the operation will be performed
 * @param op_stream op_stream associated with the operation
 * @param attr optional attributes to customize operation
 * @param deps optional vector of events that the operation should depend on
 * @return @ref ccl::event object to track the progress of the operation
 */
event alltoallv(const void* send_buf,
                  const vector_class<size_t>& send_counts,
                  void* recv_buf,
                  const vector_class<size_t>& recv_counts,
                  datatype dtype,
                  const communicator& comm,
                  stream& op_stream = default_stream,
                  const alltoallv_attr& attr = default_alltoallv_attr,
                  const vector_class<event>& deps = {});

/**
 * @param send_bufs array of buffers to store send blocks, one buffer per each rank
 * @param send_counts array with number of elements of type @c dtype in send blocks for each rank
 * @param recv_bufs [out] array of buffers to store receive blocks, one buffer per each rank
 * @param recv_counts array with number of elements of type @c dtype in receive blocks from each rank
 * @param dtype datatype of elements in @c send_buf and @c recv_buf
 * @param comm the communicator for which the operation will be performed
 * @param op_stream op_stream associated with the operation
 * @param attr optional attributes to customize operation
 * @param deps optional vector of events that the operation should depend on
 * @return @ref ccl::event object to track the progress of the operation
 */
event alltoallv(const vector_class<void*>& send_bufs,
                  const vector_class<size_t>& send_counts,
                  const vector_class<void*>& recv_bufs,
                  const vector_class<size_t>& recv_counts,
                  datatype dtype,
                  const communicator& comm,
                  stream& op_stream = default_stream,
                  const alltoallv_attr& attr = default_alltoallv_attr,
                  const vector_class<event>& deps = {});

/**
 * Type safety version:
 * @param send_buf the buffer with elements of @c BufferType that stores local blocks to be sent to each rank
 * @param send_counts array with number of elements of type @c BufferType in send blocks for each rank
 * @param recv_buf [out] the buffer to store received result, should be large enough to hold blocks from all ranks
 * @param recv_counts array with number of elements of type @c BufferType in receive blocks from each rank
 * @param comm the communicator for which the operation will be performed
 * @param op_stream op_stream associated with the operation
 * @param attr optional attributes to customize operation
 * @param deps optional vector of events that the operation should depend on
 * @return @ref ccl::event object to track the progress of the operation
 */
template <class BufferType,
          class = typename std::enable_if<is_native_type_supported<BufferType>(), event>::type>
event alltoallv(const BufferType* send_buf,
                  const vector_class<size_t>& send_counts,
                  BufferType* recv_buf,
                  const vector_class<size_t>& recv_counts,
                  const communicator& comm,
                  stream& op_stream = default_stream,
                  const alltoallv_attr& attr = default_alltoallv_attr,
                  const vector_class<event>& deps = {});

/**
 * Type safety version:
 * @param send_bufs array of buffers to store send blocks, one buffer per each rank
 * @param send_counts array with number of elements of type @c BufferType in send blocks for each rank
 * @param recv_bufs [out] array of buffers to store receive blocks, one buffer per each rank
 * @param recv_counts array with number of elements of type @c BufferType in receive blocks from each rank
 * @param comm the communicator for which the operation will be performed
 * @param op_stream op_stream associated with the operation
 * @param attr optional attributes to customize operation
 * @param deps optional vector of events that the operation should depend on
 * @return @ref ccl::event object to track the progress of the operation
 */
template <class BufferType,
          class = typename std::enable_if<is_native_type_supported<BufferType>(), event>::type>
event alltoallv(const vector_class<BufferType*>& send_bufs,
                  const vector_class<size_t>& send_counts,
                  const vector_class<BufferType*>& recv_bufs,
                  const vector_class<size_t>& recv_counts,
                  const communicator& comm,
                  stream& op_stream = default_stream,
                  const alltoallv_attr& attr = default_alltoallv_attr,
                  const vector_class<event>& deps = {});

/**
 * Type safety version:
 * @param send_buf the buffer with elements of @c dtype that stores local blocks to be sent to each rank
 * @param send_counts array with number of elements of type @c dtype in send blocks for each rank
 * @param recv_buf [out] the buffer to store received result, should be large enough to hold blocks from all ranks
 * @param recv_counts array with number of elements of type @c dtype in receive blocks from each rank
 * @param comm the communicator for which the operation will be performed
 * @param op_stream op_stream associated with the operation
 * @param attr optional attributes to customize operation
 * @param deps optional vector of events that the operation should depend on
 * @return @ref ccl::event object to track the progress of the operation
 */
template <class BufferObjectType,
          class = typename std::enable_if<is_class_supported<BufferObjectType>(), event>::type>
event alltoallv(const BufferObjectType& send_buf,
                  const vector_class<size_t>& send_counts,
                  BufferObjectType& recv_buf,
                  const vector_class<size_t>& recv_counts,
                  const communicator& comm,
                  stream& op_stream = default_stream,
                  const alltoallv_attr& attr = default_alltoallv_attr,
                  const vector_class<event>& deps = {});

/**
 * Type safety version:
 * @param send_bufs array of buffers to store send blocks, one buffer per each rank
 * @param send_counts array with number of elements of type @c dtype in send blocks for each rank
 * @param recv_bufs [out] array of buffers to store receive blocks, one buffer per each rank
 * @param recv_counts array with number of elements of type @c dtype in receive blocks from each rank
 * @param dtype datatype of elements in @c send_buf and @c recv_buf
 * @param comm the communicator for which the operation will be performed
 * @param op_stream op_stream associated with the operation
 * @param attr optional attributes to customize operation
 * @param deps optional vector of events that the operation should depend on
 * @return @ref ccl::event object to track the progress of the operation
 */
template <class BufferObjectType,
          class = typename std::enable_if<is_class_supported<BufferObjectType>(), event>::type>
event alltoallv(const vector_class<reference_wrapper_class<BufferObjectType>>& send_bufs,
                  const vector_class<size_t>& send_counts,
                  const vector_class<reference_wrapper_class<BufferObjectType>>& recv_bufs,
                  const vector_class<size_t>& recv_counts,
                  const communicator& comm,
                  stream& op_stream = default_stream,
                  const alltoallv_attr& attr = default_alltoallv_attr,
                  const vector_class<event>& deps = {});

/**
 * Barrier synchronization across all ranks of communicator.
 * Completes after all ranks in the communicator have called it.
 *
 * @param comm the communicator for which the operation will be performed
 * @param op_stream op_stream associated with the operation
 * @param attr optional attributes to customize operation
 * @param deps optional vector of events that the operation should depend on
 * @return @ref ccl::event object to track the progress of the operation
 */
event barrier(const communicator& comm,
                stream& op_stream = default_stream,
                const barrier_attr& attr = default_barrier_attr,
                const vector_class<event>& deps = {});

/**
 * Broadcast is collective communication operation that broadcasts data
 * from one rank of communicator (denoted as root) to all other ranks.
 */

/**
 * @param buf [in,out] the buffer with @c count elements of @c dtype
 * serves as send buffer for root and as receive buffer for other ranks
 * @param count number of elements of type @c dtype in @c buf
 * @param dtype datatype of elements in @c buf
 * @param root the rank that broadcasts @c buf
 * @param comm the communicator for which the operation will be performed
 * @param op_stream op_stream associated with the operation
 * @param attr optional attributes to customize operation
 * @param deps optional vector of events that the operation should depend on
 * @return @ref ccl::event object to track the progress of the operation
 */
event broadcast(void* buf,
                  size_t count,
                  datatype dtype,
                  size_t root,
                  const communicator& comm,
                  stream& op_stream = default_stream,
                  const broadcast_attr& attr = default_broadcast_attr,
                  const vector_class<event>& deps = {});

/**
 * Type safety version:
 * @param buf [in,out] the buffer with @c count elements of @c BufferType
 * serves as send buffer for root and as receive buffer for other ranks
 * @param count number of elements of type @c BufferType in @c buf
 * @param root the rank that broadcasts @c buf
 * @param comm the communicator for which the operation will be performed
 * @param op_stream op_stream associated with the operation
 * @param attr optional attributes to customize operation
 * @param deps optional vector of events that the operation should depend on
 * @return @ref ccl::event object to track the progress of the operation
 */
template <class BufferType,
          class = typename std::enable_if<is_native_type_supported<BufferType>(), event>::type>
event broadcast(BufferType* buf,
                  size_t count,
                  size_t root,
                  const communicator& comm,
                  stream& op_stream = default_stream,
                  const broadcast_attr& attr = default_broadcast_attr,
                  const vector_class<event>& deps = {});

/**
 * Type safety version:
 * @param buf [in,out] the buffer with @c count elements of @c dtype
 * serves as send buffer for root and as receive buffer for other ranks
 * @param count number of elements of type @c dtype in @c buf
 * @param root the rank that broadcasts @c buf
 * @param comm the communicator for which the operation will be performed
 * @param op_stream op_stream associated with the operation
 * @param attr optional attributes to customize operation
 * @param deps optional vector of events that the operation should depend on
 * @return @ref ccl::event object to track the progress of the operation
 */
template <class BufferObjectType,
          class = typename std::enable_if<is_class_supported<BufferObjectType>(), event>::type>
event broadcast(BufferObjectType& buf,
                  size_t count,
                  size_t root,
                  const communicator& comm,
                  stream& op_stream = default_stream,
                  const broadcast_attr& attr = default_broadcast_attr,
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
 * @param rtype type of reduction operation to be applied
 * @param root the rank that gets the result of reduction
 * @param comm the communicator for which the operation will be performed
 * @param op_stream op_stream associated with the operation
 * @param attr optional attributes to customize operation
 * @param deps optional vector of events that the operation should depend on
 * @return @ref ccl::event object to track the progress of the operation
 */
event reduce(const void* send_buf,
               void* recv_buf,
               size_t count,
               datatype dtype,
               reduction rtype,
               size_t root,
               const communicator& comm,
               stream& op_stream = default_stream,
               const reduce_attr& attr = default_reduce_attr,
               const vector_class<event>& deps = {});

/**
 * Type safety version:
 * @param send_buf the buffer with @c count elements of @c BufferType that stores local data to be reduced
 * @param recv_buf [out] the buffer to store reduced result, must have the same dimension as @c send_buf.
 * Used by the @c root rank only, ignored by other ranks.
 * @param count number of elements of type @c BufferType in @c send_buf and @c recv_buf
 * @param rtype type of reduction operation to be applied
 * @param root the rank that gets the result of reduction
 * @param comm the communicator for which the operation will be performed
 * @param op_stream op_stream associated with the operation
 * @param attr optional attributes to customize operation
 * @param deps optional vector of events that the operation should depend on
 * @return @ref ccl::event object to track the progress of the operation
 */
template <class BufferType,
          class = typename std::enable_if<is_native_type_supported<BufferType>(), event>::type>
event reduce(const BufferType* send_buf,
               BufferType* recv_buf,
               size_t count,
               reduction rtype,
               size_t root,
               const communicator& comm,
               stream& op_stream = default_stream,
               const reduce_attr& attr = default_reduce_attr,
               const vector_class<event>& deps = {});

/**
 * Type safety version:
 * @param send_buf the buffer with @c count elements of @c dtype that stores local data to be reduced
 * @param recv_buf [out] the buffer to store reduced result, must have the same dimension as @c send_buf.
 * Used by the @c root rank only, ignored by other ranks.
 * @param count number of elements of type @c dtype in @c send_buf and @c recv_buf
 * @param dtype datatype of elements in @c send_buf and @c recv_buf
 * @param rtype type of reduction operation to be applied
 * @param comm the communicator for which the operation will be performed
 * @param op_stream op_stream associated with the operation
 * @param attr optional attributes to customize operation
 * @param deps optional vector of events that the operation should depend on
 * @return @ref ccl::event object to track the progress of the operation
 */
template <class BufferObjectType,
          class = typename std::enable_if<is_class_supported<BufferObjectType>(), event>::type>
event reduce(const BufferObjectType& send_buf,
               BufferObjectType& recv_buf,
               size_t count,
               reduction rtype,
               size_t root,
               const communicator& comm,
               stream& op_stream = default_stream,
               const reduce_attr& attr = default_reduce_attr,
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
 * @param rtype type of reduction operation to be applied
 * @param comm the communicator for which the operation will be performed
 * @param op_stream op_stream associated with the operation
 * @param attr optional attributes to customize operation
 * @param deps optional vector of events that the operation should depend on
 * @return @ref ccl::event object to track the progress of the operation
 */
event reduce_scatter(const void* send_buf,
                       void* recv_buf,
                       size_t recv_count,
                       datatype dtype,
                       reduction rtype,
                       const communicator& comm,
                       stream& op_stream = default_stream,
                       const reduce_scatter_attr& attr = default_reduce_scatter_attr,
                       const vector_class<event>& deps = {});

/**
 * Type safety version:
 * @param send_buf the buffer with @c comm_size * @c count elements of @c BufferType that stores local data to be reduced
 * @param recv_buf [out] the buffer to store result block containing @c recv_count elements of type @c BufferType
 * @param recv_count number of elements of type @c BufferType in receive block
 * @param rtype type of reduction operation to be applied
 * @param comm the communicator for which the operation will be performed
 * @param op_stream op_stream associated with the operation
 * @param attr optional attributes to customize operation
 * @param deps optional vector of events that the operation should depend on
 * @return @ref ccl::event object to track the progress of the operation
 */
template <class BufferType,
          class = typename std::enable_if<is_native_type_supported<BufferType>(), event>::type>
event reduce_scatter(const BufferType* send_buf,
                       BufferType* recv_buf,
                       size_t recv_count,
                       reduction rtype,
                       const communicator& comm,
                       stream& op_stream = default_stream,
                       const reduce_scatter_attr& attr = default_reduce_scatter_attr,
                       const vector_class<event>& deps = {});

/**
 * Type safety version:
 * @param send_buf the buffer with @c comm_size * @c count elements of @c dtype that stores local data to be reduced
 * @param recv_buf [out] the buffer to store result block containing @c recv_count elements of type @c dtype
 * @param recv_count number of elements of type @c dtype in receive block
 * @param rtype type of reduction operation to be applied
 * @param comm the communicator for which the operation will be performed
 * @param op_stream op_stream associated with the operation
 * @param attr optional attributes to customize operation
 * @param deps optional vector of events that the operation should depend on
 * @return @ref ccl::event object to track the progress of the operation
 */
template <class BufferObjectType,
          class = typename std::enable_if<is_class_supported<BufferObjectType>(), event>::type>
event reduce_scatter(const BufferObjectType& send_buf,
                       BufferObjectType& recv_buf,
                       size_t recv_count,
                       reduction rtype,
                       const communicator& comm,
                       stream& op_stream = default_stream,
                       const reduce_scatter_attr& attr = default_reduce_scatter_attr,
                       const vector_class<event>& deps = {});

namespace preview {

/**
 * Sparse allreduce is a collective communication operation that makes global reduction operation
 * on sparse buffers from all ranks of communicator and distributes result back to all ranks.
 * Sparse buffers are defined by separate index and value buffers.
 */

/**
 * @param send_ind_buf the buffer of indices with @c send_ind_count elements of type @c ind_dtype
 * @param send_ind_count number of elements of type @c ind_type @c send_ind_buf
 * @param send_val_buf the buffer of values with @c send_val_count elements of type @c val_dtype
 * @param send_val_count number of elements of type @c val_type @c send_val_buf
 * @param recv_ind_buf [out] the buffer to store reduced indices, unused
 * @param recv_ind_count [out] number of elements in @c recv_ind_buf, unused
 * @param recv_val_buf [out] the buffer to store reduced values, unused
 * @param recv_val_count [out] number of elements in @c recv_val_buf, unused
 * @param ind_dtype datatype of elements in @c send_ind_buf and @c recv_ind_buf
 * @param val_dtype datatype of elements in @c send_val_buf and @c recv_val_buf
 * @param rtype type of reduction operation to be applied
 * @param comm the communicator for which the operation will be performed
 * @param op_stream op_stream associated with the operation
 * @param attr optional attributes to customize operation
 * @param deps optional vector of events that the operation should depend on
 * @return @ref ccl::event object to track the progress of the operation
 */

ccl::event sparse_allreduce(
    const void* send_ind_buf,
    size_t send_ind_count,
    const void* send_val_buf,
    size_t send_val_count,
    void* recv_ind_buf,
    size_t recv_ind_count,
    void* recv_val_buf,
    size_t recv_val_count,
    ccl::datatype ind_dtype,
    ccl::datatype val_dtype,
    ccl::reduction rtype,
    const ccl::communicator& comm,
    ccl::stream& op_stream = ccl::default_stream,
    const ccl::sparse_allreduce_attr& attr = ccl::default_sparse_allreduce_attr,
    const ccl::vector_class<ccl::event>& deps = {});

/**
 * Type safety version:
 * @param send_ind_buf the buffer of indices with @c send_ind_count elements of type @c ind_dtype
 * @param send_ind_count number of elements of type @c ind_type @c send_ind_buf
 * @param send_val_buf the buffer of values with @c send_val_count elements of type @c val_dtype
 * @param send_val_count number of elements of type @c val_type @c send_val_buf
 * @param recv_ind_buf [out] the buffer to store reduced indices, unused
 * @param recv_ind_count [out] number of elements in @c recv_ind_buf, unused
 * @param recv_val_buf [out] the buffer to store reduced values, unused
 * @param recv_val_count [out] number of elements in @c recv_val_buf, unused
 * @param rtype type of reduction operation to be applied
 * @param comm the communicator for which the operation will be performed
 * @param op_stream op_stream associated with the operation
 * @param attr optional attributes to customize operation
 * @param deps optional vector of events that the operation should depend on
 * @return @ref ccl::event object to track the progress of the operation
 */
template <class IndexBufferType,
          class ValueBufferType,
          class = typename std::enable_if<ccl::is_native_type_supported<ValueBufferType>(),
                                          ccl::event>::type>
ccl::event sparse_allreduce(
    const IndexBufferType* send_ind_buf,
    size_t send_ind_count,
    const ValueBufferType* send_val_buf,
    size_t send_val_count,
    IndexBufferType* recv_ind_buf,
    size_t recv_ind_count,
    ValueBufferType* recv_val_buf,
    size_t recv_val_count,
    ccl::reduction rtype,
    const ccl::communicator& comm,
    ccl::stream& op_stream = default_stream,
    const ccl::sparse_allreduce_attr& attr = default_sparse_allreduce_attr,
    const ccl::vector_class<ccl::event>& deps = {});

// /**
//  * Type safety version:
//  * @param send_ind_buf the buffer of indices with @c send_ind_count elements of type @c ind_dtype
//  * @param send_ind_count number of elements of type @c ind_type @c send_ind_buf
//  * @param send_val_buf the buffer of values with @c send_val_count elements of type @c val_dtype
//  * @param send_val_count number of elements of type @c val_type @c send_val_buf
//  * @param recv_ind_buf [out] the buffer to store reduced indices, unused
//  * @param recv_ind_count [out] number of elements in @c recv_ind_buf, unused
//  * @param recv_val_buf [out] the buffer to store reduced values, unused
//  * @param recv_val_count [out] number of elements in @c recv_val_buf, unused
//  * @param rtype type of reduction operation to be applied
//  * @param comm the communicator for which the operation will be performed
//  * @param op_stream op_stream associated with the operation
//  * @param attr optional attributes to customize operation
//  * @param deps optional vector of events that the operation should depend on
//  * @return @ref ccl::event object to track the progress of the operation
//  */
// template <class IndexBufferObjectType,
//           class ValueBufferObjectType,
//           class = typename std::enable_if<ccl::is_native_type_supported<ValueBufferObjectType>(),
//                                           ccl::event>::type>
// ccl::event
// sparse_allreduce(const IndexBufferObjectType& send_ind_buf,
//                  size_t send_ind_count,
//                  const ValueBufferObjectType& send_val_buf,
//                  size_t send_val_count,
//                  IndexBufferObjectType& recv_ind_buf,
//                  size_t recv_ind_count,
//                  ValueBufferObjectType& recv_val_buf,
//                  size_t recv_val_count,
//                  ccl::reduction reduction,
//                  const ccl::communicator& comm,
//                  ccl::stream& op_stream = default_stream,
//                  const ccl::sparse_allreduce_attr& attr = default_sparse_allreduce_attr,
//                  const ccl::vector_class<ccl::event>& deps = {});

} // namespace preview

// #endif //#if defined(MULTI_GPU_SUPPORT) || defined(CCL_ENABLE_SYCL)

} // namespace ccl
