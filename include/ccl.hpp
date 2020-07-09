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

/**
 * Types which allow to operate with kvs/communicator/request/stream objects in RAII manner
 */
using kvs_interface_t = std::unique_ptr<kvs_interface>;
using communicator_t = std::unique_ptr<communicator>;
using request_t = std::unique_ptr<request>;

#ifdef DEVICE_COMM_SUPPORT
using device_communicator_t = std::unique_ptr<device_communicator>;
using device_request_t = std::unique_ptr<device_request>;
using stream_t = std::unique_ptr<stream>;
#endif /* DEVICE_COMM_SUPPORT */

class kvs_interface {
public:
    virtual bool get(const std::string& prefix,
                     const std::string& key,
                     std::vector<char>& result) const = 0;

    virtual void put(const std::string& prefix,
                     const std::string& key,
                     const std::vector<char>& data) const = 0;

    virtual ~kvs_interface() = default;
};

class master_kvs final : public kvs_interface {
public:
    static constexpr size_t addr_max_size = 256;
    using addr_t = std::array<char, addr_max_size>;

    master_kvs();
    ~master_kvs() override;

    bool get(const std::string& prefix,
             const std::string& key,
             std::vector<char>& result) const override;

    void put(const std::string& prefix,
             const std::string& key,
             const std::vector<char>& data) const override;

    const addr_t& get_addr() const;

private:
    std::unique_ptr<master_kvs_impl> pimpl;
};

class kvs final : public kvs_interface {
public:
    using addr_t = master_kvs::addr_t;

    kvs(const addr_t& addr);
    ~kvs() override;

    bool get(const std::string& prefix,
             const std::string& key,
             std::vector<char>& result) const override;

    void put(const std::string& prefix,
             const std::string& key,
             const std::vector<char>& data) const override;

private:
    std::unique_ptr<kvs_impl> pimpl;
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

    /**
     * Creates a new master key-value store.
     * It's address should be distributed to other ranks
     * using out of band communication transport and be used to create non-master key-value stores.
     * @return kvs master
     */
    master_kvs_t create_master_kvs() const;

    /**
     * Creates a new key-value store
     * @param master_addr address of master kvs
     * @return kvs master
     */
    kvs_t create_kvs(const master_kvs_addr& addr) const;

    /**
     * Creates a new host communicator with externally defined size, rank and kvs (e.g. in case of mpirun launcher).
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
    communicator_t create_communicator(const size_t size, std::shared_ptr<kvs_interface> kvs) const;

    /**
     * Creates a new host communicator with user supplied size, rank and kvs.
     * @param size user-supplied total number of ranks
     * @param rank user-supplied rank
     * @param kvs key-value store for ranks wire-up
     * @return host communicator
     */
    communicator_t create_communicator(const size_t size,
                                       const size_t rank,
                                       std::shared_ptr<kvs_interface> kvs) const;

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
     * @param kvs key-value store for ranks wire-up
     * @return vector of device communicators
     */
    std::vector<device_communicator_t> create_device_communicators(
        const size_t size,
        const std::vector<device_native_type>& devices,
        std::shared_ptr<kvs_interface> kvs) const;

    /**
     * Creates a new device communicators with user supplied size, ranks, device indices and kvs.
     * @param size user-supplied total number of ranks
     * @param rank_device_map user-supplied mapping of local ranks on devices
     * @param kvs key-value store for ranks wire-up
     * @return vector of device communicators
     */
    std::vector<device_communicator_t> create_device_communicators(
        const size_t size,
        std::vector<std::pair<size_t, device_native_type>>& rank_device_map,
        std::shared_ptr<kvs_interface> kvs) const;

    /**
     * Creates @attr which used to split device communicator
     */
    device_comm_split_attr_t create_device_comm_split_attr() const;

    /**
     * Splits device communicators according to attributes.
     * @param attrs split attributes for local communicators
     * @return vector of device communicators
     */
    std::vector<device_communicator_t> split_device_communicators(
        const std::vector<std::pair<device_communicator_t, device_comm_split_attr_t>>& attrs) const;

    /**
     * Creates a new stream from @native_stream_type
     * @param native_stream the existing handle of stream
     * @args  full parameters set
     */
    template <class native_stream_type,
              class = typename std::enable_if<is_stream_supported<native_stream_type>()>::type>
    stream_t create_stream(
        native_stream_type& native_stream,
        const info::stream_properties_data_t& args = info::stream_properties_data_t{});

    /**
     * Creates a new native_stream from @native_stream_type
     * @param native_stream the existing handle of stream
     * @props are optional specific properties
     */
    template <class native_stream_type,
              class = typename std::enable_if<is_stream_supported<native_stream_type>()>::type,
              info::stream_properties... prop_ids>
    stream_t create_stream(native_stream_type& native_stream,
                           info::arg<prop_ids, info::stream_property_value_t>... props) {
        using tuple_t = std::tuple<info::stream_property_value_t>... > ;
        tuple_t args{ props... };

        info::stream_properties_data_t stream_args;
        arg_filler exec(args);
        ccl_tuple_for_each(args, arg_filler);
        return create_event(native_stream, stream_args);
    }

    /* Example:
     *  auto stream = ccl::environment::instance().create_stream(queue,
     *                                                           stream_arg(info::stream_properties::ordinal, 0),
     *                                                           stream_arg(info::stream_properties::index, 1),
     *                                                           stream_arg(info::stream_properties::mode, ZE_ASYNC));
     *
     */
    stream_t create_stream() const;

    /**
     * Creates a new event from @native_event_type
     * @param native_event the existing handle of event
     */
    template <class native_event_type,
              class = typename std::enable_if<is_event_supported<native_event_type>()>::type>
    event_t create_event(native_event_type& native_event);

#endif /* DEVICE_COMM_SUPPORT */

private:
    environment();
};

/**
 * An request interface that allows the user to track communication operation progress
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
     * Retrieves the rank of the current process in a communicator
     * @return rank of the current process
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
     * Allgatherv is a collective communication operation that collects data from all ranks within a communicator.
     * Each rank gets the same result data. Different ranks can contribute segments of different sizes.
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
                         const std::vector<size_t>& recv_counts,
                         datatype dtype,
                         const allgatherv_attr_t& attr = allgatherv_attr_t());

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
                         const std::vector<void*>& recv_bufs,
                         const std::vector<size_t>& recv_counts,
                         datatype dtype,
                         const allgatherv_attr_t& attr = allgatherv_attr_t());
    /**
     * Type safety version:
     * @param send_buf the buffer with @c send_count elements of @c dtype that stores local data to be gathered
     * @param send_count number of elements of type @c dtype in @c send_buf
     * @param recv_buf [out] the buffer to store gathered result, should be large enough to hold values from all ranks
     * @param recv_counts array with number of elements of type @c dtype to be received from each rank
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    template <class buffer_type,
              class = typename std::enable_if<ccl::is_native_type_supported<buffer_type>()>::type>
    request_t allgatherv(const buffer_type* send_buf,
                         size_t send_count,
                         buffer_type* recv_buf,
                         const std::vector<size_t>& recv_counts,
                         const allgatherv_attr_t& attr = allgatherv_attr_t());

    /**
     * Type safety version:
     * @param send_buf the buffer with @c send_count elements of @c dtype that stores local data to be gathered
     * @param send_count number of elements of type @c dtype in @c send_buf
     * @param recv_bufs [out] array of buffers to store gathered result, one buffer per each rank
     * @param recv_counts array with number of elements of type @c dtype to be received from each rank
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    template <class buffer_type,
              class = typename std::enable_if<ccl::is_native_type_supported<buffer_type>()>::type>
    request_t allgatherv(const buffer_type* send_buf,
                         size_t send_count,
                         std::vector<buffer_type*>& recv_bufs,
                         const std::vector<size_t>& recv_counts,
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
     * @param send_buf the buffer with @c count elements of @c dtype that stores local data to be reduced
     * @param recv_buf [out] the buffer to store reduced result, must have the same dimension as @c send_buf
     * @param count number of elements of type @c dtype in @c send_buf and @c recv_buf
     * @param reduction type of reduction operation to be applied
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    template <class buffer_type,
              class = typename std::enable_if<ccl::is_native_type_supported<buffer_type>()>::type>
    request_t allreduce(const buffer_type* send_buf,
                        buffer_type* recv_buf,
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
    request_t alltoall(const std::vector<void*>& send_buf,
                       const std::vector<void*>& recv_buf,
                       size_t count,
                       datatype dtype,
                       const alltoall_attr_t& attr = alltoall_attr_t());

    /**
     * Type safety version:
     * @param send_buf the buffer with @c count elements of @c dtype that stores local data to be sent
     * @param recv_buf [out] the buffer to store received result, should be large enough
     * to hold values from all ranks, i.e. at least @c comm_size * @c count
     * @param count number of elements to be send to or to received from each rank
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    template <class buffer_type,
              class = typename std::enable_if<ccl::is_native_type_supported<buffer_type>()>::type>
    request_t alltoall(const buffer_type* send_buf,
                       buffer_type* recv_buf,
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
    template <class buffer_type,
              class = typename std::enable_if<ccl::is_native_type_supported<buffer_type>()>::type>
    request_t alltoall(const std::vector<buffer_type*>& send_buf,
                       const std::vector<buffer_type*>& recv_buf,
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
                        const std::vector<size_t>& send_counts,
                        void* recv_buf,
                        const std::vector<size_t>& recv_counts,
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
    request_t alltoallv(const std::vector<void*>& send_bufs,
                        const std::vector<size_t>& send_counts,
                        const std::vector<void*>& recv_bufs,
                        const std::vector<size_t>& recv_counts,
                        datatype dtype,
                        const alltoallv_attr_t& attr = alltoallv_attr_t());

    /**
     * Type safety version:
     * @param send_buf the buffer with elements of @c dtype that stores local blocks to be sent to each rank
     * @param send_counts array with number of elements of type @c dtype in send blocks for each rank
     * @param recv_buf [out] the buffer to store received result, should be large enough to hold blocks from all ranks
     * @param recv_counts array with number of elements of type @c dtype in receive blocks from each rank
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    template <class buffer_type,
              class = typename std::enable_if<ccl::is_native_type_supported<buffer_type>()>::type>
    request_t alltoallv(const buffer_type* send_buf,
                        const std::vector<size_t>& send_counts,
                        buffer_type* recv_buf,
                        const std::vector<size_t>& recv_counts,
                        const alltoallv_attr_t& attr = alltoallv_attr_t());

    /**
     * Type safety version:
     * @param send_bufs array of buffers to store send blocks, one buffer per each rank
     * @param send_counts array with number of elements of type @c dtype in send blocks for each rank
     * @param recv_bufs [out] array of buffers to store receive blocks, one buffer per each rank
     * @param recv_counts array with number of elements of type @c dtype in receive blocks from each rank
     * @param dtype datatype of elements in @c send_buf and @c recv_buf
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    template <class buffer_type,
              class = typename std::enable_if<ccl::is_native_type_supported<buffer_type>()>::type>
    request_t alltoallv(const std::vector<buffer_type*>& send_bufs,
                        const std::vector<size_t>& send_counts,
                        const std::vector<buffer_type*>& recv_bufs,
                        const std::vector<size_t>& recv_counts,
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
     * @param buf [in,out] the buffer with @c count elements of @c dtype
     * serves as send buffer for root and as receive buffer for other ranks
     * @param count number of elements of type @c dtype in @c buf
     * @param root the rank that broadcasts @c buf
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    template <class buffer_type,
              class = typename std::enable_if<ccl::is_native_type_supported<buffer_type>()>::type>
    request_t bcast(buffer_type* buf,
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
     * @param send_buf the buffer with @c count elements of @c dtype that stores local data to be reduced
     * @param recv_buf [out] the buffer to store reduced result, must have the same dimension as @c send_buf.
     * Used by the @c root rank only, ignored by other ranks.
     * @param count number of elements of type @c dtype in @c send_buf and @c recv_buf
     * @param dtype datatype of elements in @c send_buf and @c recv_buf
     * @param reduction type of reduction operation to be applied
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    template <class buffer_type,
              class = typename std::enable_if<ccl::is_native_type_supported<buffer_type>()>::type>
    request_t reduce(const buffer_type* send_buf,
                     buffer_type* recv_buf,
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
     * @param send_buf the buffer with @c comm_size * @c count elements of @c dtype that stores local data to be reduced
     * @param recv_buf [out] the buffer to store result block containing @c recv_count elements of type @c dtype
     * @param recv_count number of elements of type @c dtype in receive block
     * @param reduction type of reduction operation to be applied
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    template <class buffer_type,
              class = typename std::enable_if<ccl::is_native_type_supported<buffer_type>()>::type>
    request_t reduce_scatter(const buffer_type* send_buf,
                             buffer_type* recv_buf,
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
        class index_buffer_type,
        class value_buffer_type,
        class = typename std::enable_if<ccl::is_native_type_supported<value_buffer_type>()>::type>
    request_t sparse_allreduce(const index_buffer_type* send_ind_buf,
                               size_t send_ind_count,
                               const value_buffer_type* send_val_buf,
                               size_t send_val_count,
                               index_buffer_type* recv_ind_buf,
                               size_t recv_ind_count,
                               value_buffer_type* recv_val_buf,
                               size_t recv_val_count,
                               reduction reduction,
                               const sparse_allreduce_attr_t& attr = sparse_allreduce_attr_t());

private:
    friend class environment;

    explicit communicator(std::shared_ptr<communicator_interface> impl);

    /**
     * Holds implementation specific details
     */
    std::shared_ptr<communicator_interface> pimpl;
};

#ifdef DEVICE_COMM_SUPPORT
/**
 * An device operation interface that allows the user to track operation progress
 */
class device_request : public request {
public:
    /**
     * Retrieves the event object
     */
    virtual event_t get_event() = 0;
};

/**
 * A event object is an abstraction over stream events
 */
class event : public non_copyable<event>,
              public non_movable<event>,
              public pointer_on_impl<event, ccl_event> {
public:
    using native_handle_t = typename unified_event_type::native_reference_t;
    using impl_value_t = typename pointer_on_impl<event, ccl_event>::impl_value_t;

    event(native_event_type& native_event);

    native_handle_t get() const const native_handle_t& get() const;

private:
    friend class communicator;
    friend class environment;
    event(impl_value_t&& impl);
};

class device_communicator final
        : public pointer_on_impl<device_communicator, communicator_interface> {
public:
    using native_handle_t = typename unified_device_type::native_reference_t;
    using impl_value_t =
        typename pointer_on_impl<device_communicator, communicator_interface>::impl_value_t;
    ~device_communicator();

    /**
     * Retrieves the rank of the current process in a communicator
     * @return rank of the current process
     */
    size_t rank() const;

    /**
     * Retrieves the number of processes in a communicator
     * @return number of the processes
     */
    size_t size() const;

    /**
     * Type allows to get underlying device type,
     * which was used as communicator construction argument
     */
    using native_device_reference_t = native_handle_t;

    /**
     * Retrieves underlying device, which was used as communicator construction argument
     */
    native_device_reference_t device();

    device_communicator split(device_comm_split_attr_t attr);

    /**
     * Allgatherv is a collective communication operation that collects data from all ranks within a communicator.
     * Each rank gets the same result data. Different ranks can contribute segments of different sizes.
     */

    /**
     * @param send_buf the buffer with @c send_count elements of @c dtype that stores local data to be gathered
     * @param send_count number of elements of type @c dtype in @c send_buf
     * @param recv_buf [out] the buffer to store gathered result, should be large enough to hold values from all ranks
     * @param recv_counts array with number of elements of type @c dtype to be received from each rank
     * @param dtype datatype of elements in @c send_buf and @c recv_buf
     * @param stream optional stream associated with the operation
     * @param deps optional vector of events that the execution should depend on
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    request_t allgatherv(const void* send_buf,
                         size_t send_count,
                         void* recv_buf,
                         const std::vector<size_t>& recv_counts,
                         datatype dtype,
                         const stream_t& stream = stream_t(),
                         const std::vector<event_t>& deps = {},
                         const allgatherv_attr_t& attr = allgatherv_attr_t());

    /**
     * @param send_buf the buffer with @c send_count elements of @c dtype that stores local data to be gathered
     * @param send_count number of elements of type @c dtype in @c send_buf
     * @param recv_bufs [out] array of buffers to store gathered result, one buffer per each rank
     * @param recv_counts array with number of elements of type @c dtype to be received from each rank
     * @param dtype datatype of elements in @c send_buf and @c recv_buf
     * @param stream optional stream associated with the operation
     * @param deps optional vector of events that the execution should depend on
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    request_t allgatherv(const void* send_buf,
                         size_t send_count,
                         const std::vector<void*>& recv_bufs,
                         const std::vector<size_t>& recv_counts,
                         datatype dtype,
                         const stream_t& stream = stream_t(),
                         const std::vector<event_t>& deps = {},
                         const allgatherv_attr_t& attr = allgatherv_attr_t());
    /**
     * Type safety version:
     * @param send_buf the buffer with @c send_count elements of @c dtype that stores local data to be gathered
     * @param send_count number of elements of type @c dtype in @c send_buf
     * @param recv_buf [out] the buffer to store gathered result, should be large enough to hold values from all ranks
     * @param recv_counts array with number of elements of type @c dtype to be received from each rank
     * @param stream optional stream associated with the operation
     * @param deps optional vector of events that the execution should depend on
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    template <class buffer_type,
              class = typename std::enable_if<ccl::is_native_type_supported<buffer_type>()>::type>
    request_t allgatherv(const buffer_type* send_buf,
                         size_t send_count,
                         buffer_type* recv_buf,
                         const std::vector<size_t>& recv_counts,
                         const stream_t& stream = stream_t(),
                         const std::vector<event_t>& deps = {},
                         const allgatherv_attr_t& attr = allgatherv_attr_t());

    /**
     * Type safety version:
     * @param send_buf the buffer with @c send_count elements of @c dtype that stores local data to be gathered
     * @param send_count number of elements of type @c dtype in @c send_buf
     * @param recv_bufs [out] array of buffers to store gathered result, one buffer per each rank
     * @param recv_counts array with number of elements of type @c dtype to be received from each rank
     * @param stream optional stream associated with the operation
     * @param deps optional vector of events that the execution should depend on
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    template <class buffer_type,
              class = typename std::enable_if<ccl::is_native_type_supported<buffer_type>()>::type>
    request_t allgatherv(const buffer_type* send_buf,
                         size_t send_count,
                         std::vector<buffer_type*>& recv_bufs,
                         const std::vector<size_t>& recv_counts,
                         const stream_t& stream = stream_t(),
                         const std::vector<event_t>& deps = {},
                         const allgatherv_attr_t& attr = allgatherv_attr_t());

    /**
     * Type safety version:
     * @param send_buf the buffer with @c send_count elements of @c dtype that stores local data to be gathered
     * @param send_count number of elements of type @c dtype in @c send_buf
     * @param recv_buf [out] the buffer to store gathered result, should be large enough to hold values from all ranks
     * @param recv_counts array with number of elements of type @c dtype to be received from each rank
     * @param stream optional stream associated with the operation
     * @param deps optional vector of events that the execution should depend on
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    template <class buffer_object_type,
              class = typename std::enable_if<ccl::is_class_supported<buffer_object_type>()>::type>
    request_t allgatherv(const buffer_object_type& send_buf,
                         size_t send_count,
                         buffer_object_type& recv_buf,
                         const std::vector<size_t>& recv_counts,
                         const stream_t& stream = stream_t(),
                         const std::vector<event_t>& deps = {},
                         const allgatherv_attr_t& attr = allgatherv_attr_t());

    /**
     * Type safety version:
     * @param send_buf the buffer with @c send_count elements of @c dtype that stores local data to be gathered
     * @param send_count number of elements of type @c dtype in @c send_buf
     * @param recv_bufs [out] array of buffers to store gathered result, one buffer per each rank
     * @param recv_counts array with number of elements of type @c dtype to be received from each rank
     * @param stream optional stream associated with the operation
     * @param deps optional vector of events that the execution should depend on
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    template <class buffer_object_type,
              class = typename std::enable_if<ccl::is_class_supported<buffer_object_type>()>::type>
    request_t allgatherv(const buffer_object_type& send_buf,
                         size_t send_count,
                         std::vector<buffer_object_type&>& recv_bufs,
                         const std::vector<size_t>& recv_counts,
                         const stream_t& stream = stream_t(),
                         const std::vector<event_t>& deps = {},
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
     * @param stream optional stream associated with the operation
     * @param deps optional vector of events that the execution should depend on
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    request_t allreduce(const void* send_buf,
                        void* recv_buf,
                        size_t count,
                        datatype dtype,
                        reduction reduction,
                        const stream_t& stream = stream_t(),
                        const std::vector<event_t>& deps = {},
                        const allreduce_attr_t& attr = allreduce_attr_t());

    /**
     * Type safety version:
     * @param send_buf the buffer with @c count elements of @c dtype that stores local data to be reduced
     * @param recv_buf [out] the buffer to store reduced result, must have the same dimension as @c send_buf
     * @param count number of elements of type @c dtype in @c send_buf and @c recv_buf
     * @param reduction type of reduction operation to be applied
     * @param stream optional stream associated with the operation
     * @param deps optional vector of events that the execution should depend on
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    template <class buffer_type,
              class = typename std::enable_if<ccl::is_native_type_supported<buffer_type>()>::type>
    request_t allreduce(const buffer_type* send_buf,
                        buffer_type* recv_buf,
                        size_t count,
                        reduction reduction,
                        const stream_t& stream = stream_t(),
                        const std::vector<event_t>& deps = {},
                        const allreduce_attr_t& attr = allreduce_attr_t());

    /**
     * Type safety version:
     * @param send_buf the buffer with @c count elements of @c dtype that stores local data to be reduced
     * @param recv_buf [out] the buffer to store reduced result, must have the same dimension as @c send_buf
     * @param count number of elements of type @c dtype in @c send_buf and @c recv_buf
     * @param reduction type of reduction operation to be applied
     * @param stream optional stream associated with the operation
     * @param deps optional vector of events that the execution should depend on
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    template <class buffer_object_type,
              class = typename std::enable_if<ccl::is_class_supported<buffer_object_type>()>::type>
    request_t allreduce(const buffer_object_type& send_buf,
                        buffer_object_type& recv_buf,
                        size_t count,
                        reduction reduction,
                        const stream_t& stream = stream_t(),
                        const std::vector<event_t>& deps = {},
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
     * @param stream optional stream associated with the operation
     * @param deps optional vector of events that the execution should depend on
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    request_t alltoall(const void* send_buf,
                       void* recv_buf,
                       size_t count,
                       datatype dtype,
                       const stream_t& stream = stream_t(),
                       const std::vector<event_t>& deps = {},
                       const alltoall_attr_t& attr = alltoall_attr_t());

    /**
     * @param send_bufs array of buffers with local data to be sent, one buffer per each rank
     * @param recv_bufs [out] array of buffers to store received result, one buffer per each rank
     * @param count number of elements of type @c dtype to be send to or to received from each rank
     * @param dtype datatype of elements in @c send_buf and @c recv_buf
     * @param stream optional stream associated with the operation
     * @param deps optional vector of events that the execution should depend on
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    request_t alltoall(const std::vector<void*>& send_buf,
                       const std::vector<void*>& recv_buf,
                       size_t count,
                       datatype dtype,
                       const stream_t& stream = stream_t(),
                       const std::vector<event_t>& deps = {},
                       const alltoall_attr_t& attr = alltoall_attr_t());

    /**
     * Type safety version:
     * @param send_buf the buffer with @c count elements of @c dtype that stores local data to be sent
     * @param recv_buf [out] the buffer to store received result, should be large enough
     * to hold values from all ranks, i.e. at least @c comm_size * @c count
     * @param count number of elements to be send to or to received from each rank
     * @param stream optional stream associated with the operation
     * @param deps optional vector of events that the execution should depend on
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    template <class buffer_type,
              class = typename std::enable_if<ccl::is_native_type_supported<buffer_type>()>::type>
    request_t alltoall(const buffer_type* send_buf,
                       buffer_type* recv_buf,
                       size_t count,
                       const stream_t& stream = stream_t(),
                       const std::vector<event_t>& deps = {},
                       const alltoall_attr_t& attr = alltoall_attr_t());

    /**
     * Type safety version:
     * @param send_bufs array of buffers with local data to be sent, one buffer per each rank
     * @param recv_bufs [out] array of buffers to store received result, one buffer per each rank
     * @param count number of elements to be send to or to received from each rank
     * @param stream optional stream associated with the operation
     * @param deps optional vector of events that the execution should depend on
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    template <class buffer_type,
              class = typename std::enable_if<ccl::is_native_type_supported<buffer_type>()>::type>
    request_t alltoall(const std::vector<buffer_type*>& send_buf,
                       const std::vector<buffer_type*>& recv_buf,
                       size_t count,
                       const stream_t& stream = stream_t(),
                       const std::vector<event_t>& deps = {},
                       const alltoall_attr_t& attr = alltoall_attr_t());

    /**
     * Type safety version:
     * @param send_buf the buffer with @c count elements of @c dtype that stores local data to be sent
     * @param recv_buf [out] the buffer to store received result, should be large enough
     * to hold values from all ranks, i.e. at least @c comm_size * @c count
     * @param count number of elements to be send to or to received from each rank
     * @param stream optional stream associated with the operation
     * @param deps optional vector of events that the execution should depend on
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    template <class buffer_object_type,
              class = typename std::enable_if<ccl::is_class_supported<buffer_object_type>()>::type>
    request_t alltoall(const buffer_object_type& send_buf,
                       buffer_object_type& recv_buf,
                       size_t count,
                       const stream_t& stream = stream_t(),
                       const std::vector<event_t>& deps = {},
                       const alltoall_attr_t& attr = alltoall_attr_t());

    /**
     * Type safety version:
     * @param send_bufs array of buffers with local data to be sent, one buffer per each rank
     * @param recv_bufs [out] array of buffers to store received result, one buffer per each rank
     * @param count number of elements to be send to or to received from each rank
     * @param stream optional stream associated with the operation
     * @param deps optional vector of events that the execution should depend on
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    template <class buffer_object_type,
              class = typename std::enable_if<ccl::is_class_supported<buffer_object_type>()>::type>
    request_t alltoall(const std::vector<buffer_object_type&>& send_buf,
                       const std::vector<buffer_object_type&>& recv_buf,
                       size_t count,
                       const stream_t& stream = stream_t(),
                       const std::vector<event_t>& deps = {},
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
     * @param stream optional stream associated with the operation
     * @param deps optional vector of events that the execution should depend on
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    request_t alltoallv(const void* send_buf,
                        const std::vector<size_t>& send_counts,
                        void* recv_buf,
                        const std::vector<size_t>& recv_counts,
                        datatype dtype,
                        const stream_t& stream = stream_t(),
                        const std::vector<event_t>& deps = {},
                        const alltoallv_attr_t& attr = alltoallv_attr_t());

    /**
     * @param send_bufs array of buffers to store send blocks, one buffer per each rank
     * @param send_counts array with number of elements of type @c dtype in send blocks for each rank
     * @param recv_bufs [out] array of buffers to store receive blocks, one buffer per each rank
     * @param recv_counts array with number of elements of type @c dtype in receive blocks from each rank
     * @param dtype datatype of elements in @c send_buf and @c recv_buf
     * @param stream optional stream associated with the operation
     * @param deps optional vector of events that the execution should depend on
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    request_t alltoallv(const std::vector<void*>& send_bufs,
                        const std::vector<size_t>& send_counts,
                        const std::vector<void*>& recv_bufs,
                        const std::vector<size_t>& recv_counts,
                        datatype dtype,
                        const stream_t& stream = stream_t(),
                        const std::vector<event_t>& deps = {},
                        const alltoallv_attr_t& attr = alltoallv_attr_t());

    /**
     * Type safety version:
     * @param send_buf the buffer with elements of @c dtype that stores local blocks to be sent to each rank
     * @param send_counts array with number of elements of type @c dtype in send blocks for each rank
     * @param recv_buf [out] the buffer to store received result, should be large enough to hold blocks from all ranks
     * @param recv_counts array with number of elements of type @c dtype in receive blocks from each rank
     * @param stream optional stream associated with the operation
     * @param deps optional vector of events that the execution should depend on
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    template <class buffer_type,
              class = typename std::enable_if<ccl::is_native_type_supported<buffer_type>()>::type>
    request_t alltoallv(const buffer_type* send_buf,
                        const std::vector<size_t>& send_counts,
                        buffer_type* recv_buf,
                        const std::vector<size_t>& recv_counts,
                        const stream_t& stream = stream_t(),
                        const std::vector<event_t>& deps = {},
                        const alltoallv_attr_t& attr = alltoallv_attr_t());

    /**
     * Type safety version:
     * @param send_bufs array of buffers to store send blocks, one buffer per each rank
     * @param send_counts array with number of elements of type @c dtype in send blocks for each rank
     * @param recv_bufs [out] array of buffers to store receive blocks, one buffer per each rank
     * @param recv_counts array with number of elements of type @c dtype in receive blocks from each rank
     * @param dtype datatype of elements in @c send_buf and @c recv_buf
     * @param stream optional stream associated with the operation
     * @param deps optional vector of events that the execution should depend on
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    template <class buffer_type,
              class = typename std::enable_if<ccl::is_native_type_supported<buffer_type>()>::type>
    request_t alltoallv(const std::vector<buffer_type*>& send_bufs,
                        const std::vector<size_t>& send_counts,
                        const std::vector<buffer_type*>& recv_bufs,
                        const std::vector<size_t>& recv_counts,
                        const stream_t& stream = stream_t(),
                        const std::vector<event_t>& deps = {},
                        const alltoallv_attr_t& attr = alltoallv_attr_t());

    /**
     * Type safety version:
     * @param send_buf the buffer with elements of @c dtype that stores local blocks to be sent to each rank
     * @param send_counts array with number of elements of type @c dtype in send blocks for each rank
     * @param recv_buf [out] the buffer to store received result, should be large enough to hold blocks from all ranks
     * @param recv_counts array with number of elements of type @c dtype in receive blocks from each rank
     * @param stream optional stream associated with the operation
     * @param deps optional vector of events that the execution should depend on
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    template <class buffer_object_type,
              class = typename std::enable_if<ccl::is_class_supported<buffer_object_type>()>::type>
    request_t alltoallv(const buffer_object_type& send_buf,
                        const std::vector<size_t>& send_counts,
                        buffer_object_type& recv_buf,
                        const std::vector<size_t>& recv_counts,
                        const stream_t& stream = stream_t(),
                        const std::vector<event_t>& deps = {},
                        const alltoallv_attr_t& attr = alltoallv_attr_t());

    /**
     * Type safety version:
     * @param send_bufs array of buffers to store send blocks, one buffer per each rank
     * @param send_counts array with number of elements of type @c dtype in send blocks for each rank
     * @param recv_bufs [out] array of buffers to store receive blocks, one buffer per each rank
     * @param recv_counts array with number of elements of type @c dtype in receive blocks from each rank
     * @param dtype datatype of elements in @c send_buf and @c recv_buf
     * @param stream optional stream associated with the operation
     * @param deps optional vector of events that the execution should depend on
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    template <class buffer_object_type,
              class = typename std::enable_if<ccl::is_class_supported<buffer_object_type>()>::type>
    request_t alltoallv(const std::vector<buffer_object_type&>& send_bufs,
                        const std::vector<size_t>& send_counts,
                        const std::vector<buffer_object_type&>& recv_bufs,
                        const std::vector<size_t>& recv_counts,
                        const stream_t& stream = stream_t(),
                        const std::vector<event_t>& deps = {},
                        const alltoallv_attr_t& attr = alltoallv_attr_t());

    /**
     * Barrier synchronization across all ranks of communicator.
     * Completes after all ranks in the communicator have called it.
     *
     * @param stream optional stream associated with the operation
     * @param deps optional vector of events that the execution should depend on
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    request_t barrier(const stream_t& stream = stream_t(),
                      const std::vector<event_t>& deps = {},
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
     * @param stream optional stream associated with the operation
     * @param deps optional vector of events that the execution should depend on
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    request_t bcast(void* buf,
                    size_t count,
                    datatype dtype,
                    size_t root,
                    const stream_t& stream = stream_t(),
                    const std::vector<event_t>& deps = {},
                    const bcast_attr_t& attr = bcast_attr_t());

    /**
     * Type safety version:
     * @param buf [in,out] the buffer with @c count elements of @c dtype
     * serves as send buffer for root and as receive buffer for other ranks
     * @param count number of elements of type @c dtype in @c buf
     * @param root the rank that broadcasts @c buf
     * @param stream optional stream associated with the operation
     * @param deps optional vector of events that the execution should depend on
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    template <class buffer_type,
              class = typename std::enable_if<ccl::is_native_type_supported<buffer_type>()>::type>
    request_t bcast(buffer_type* buf,
                    size_t count,
                    size_t root,
                    const stream_t& stream = stream_t(),
                    const std::vector<event_t>& deps = {},
                    const bcast_attr_t& attr = bcast_attr_t());

    /**
     * Type safety version:
     * @param buf [in,out] the buffer with @c count elements of @c dtype
     * serves as send buffer for root and as receive buffer for other ranks
     * @param count number of elements of type @c dtype in @c buf
     * @param root the rank that broadcasts @c buf
     * @param stream optional stream associated with the operation
     * @param deps optional vector of events that the execution should depend on
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    template <class buffer_object_type,
              class = typename std::enable_if<ccl::is_class_supported<buffer_object_type>()>::type>
    request_t bcast(buffer_object_type& buf,
                    size_t count,
                    size_t root,
                    const stream_t& stream = stream_t(),
                    const std::vector<event_t>& deps = {},
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
     * @param stream optional stream associated with the operation
     * @param deps optional vector of events that the execution should depend on
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    request_t reduce(const void* send_buf,
                     void* recv_buf,
                     size_t count,
                     datatype dtype,
                     reduction reduction,
                     size_t root,
                     const stream_t& stream = stream_t(),
                     const std::vector<event_t>& deps = {},
                     const reduce_attr_t& attr = reduce_attr_t());

    /**
     * Type safety version:
     * @param send_buf the buffer with @c count elements of @c dtype that stores local data to be reduced
     * @param recv_buf [out] the buffer to store reduced result, must have the same dimension as @c send_buf.
     * Used by the @c root rank only, ignored by other ranks.
     * @param count number of elements of type @c dtype in @c send_buf and @c recv_buf
     * @param dtype datatype of elements in @c send_buf and @c recv_buf
     * @param reduction type of reduction operation to be applied
     * @param stream optional stream associated with the operation
     * @param deps optional vector of events that the execution should depend on
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    template <class buffer_type,
              class = typename std::enable_if<ccl::is_native_type_supported<buffer_type>()>::type>
    request_t reduce(const buffer_type* send_buf,
                     buffer_type* recv_buf,
                     size_t count,
                     reduction reduction,
                     size_t root,
                     const stream_t& stream = stream_t(),
                     const std::vector<event_t>& deps = {},
                     const reduce_attr_t& attr = reduce_attr_t());

    /**
     * Type safety version:
     * @param send_buf the buffer with @c count elements of @c dtype that stores local data to be reduced
     * @param recv_buf [out] the buffer to store reduced result, must have the same dimension as @c send_buf.
     * Used by the @c root rank only, ignored by other ranks.
     * @param count number of elements of type @c dtype in @c send_buf and @c recv_buf
     * @param dtype datatype of elements in @c send_buf and @c recv_buf
     * @param reduction type of reduction operation to be applied
     * @param stream optional stream associated with the operation
     * @param deps optional vector of events that the execution should depend on
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    template <class buffer_object_type,
              class = typename std::enable_if<ccl::is_class_supported<buffer_object_type>()>::type>
    request_t reduce(const buffer_object_type& send_buf,
                     buffer_object_type& recv_buf,
                     size_t count,
                     reduction reduction,
                     size_t root,
                     const stream_t& stream = stream_t(),
                     const std::vector<event_t>& deps = {},
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
     * @param stream optional stream associated with the operation
     * @param deps optional vector of events that the execution should depend on
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    request_t reduce_scatter(const void* send_buf,
                             void* recv_buf,
                             size_t recv_count,
                             datatype dtype,
                             reduction reduction,
                             const stream_t& stream = stream_t(),
                             const std::vector<event_t>& deps = {},
                             const reduce_scatter_attr_t& attr = reduce_scatter_attr_t());

    /**
     * Type safety version:
     * @param send_buf the buffer with @c comm_size * @c count elements of @c dtype that stores local data to be reduced
     * @param recv_buf [out] the buffer to store result block containing @c recv_count elements of type @c dtype
     * @param recv_count number of elements of type @c dtype in receive block
     * @param reduction type of reduction operation to be applied
     * @param stream optional stream associated with the operation
     * @param deps optional vector of events that the execution should depend on
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    template <class buffer_type,
              class = typename std::enable_if<ccl::is_native_type_supported<buffer_type>()>::type>
    request_t reduce_scatter(const buffer_type* send_buf,
                             buffer_type* recv_buf,
                             size_t recv_count,
                             reduction reduction,
                             const stream_t& stream = stream_t(),
                             const std::vector<event_t>& deps = {},
                             const reduce_scatter_attr_t& attr = reduce_scatter_attr_t());

    /**
     * Type safety version:
     * @param send_buf the buffer with @c comm_size * @c count elements of @c dtype that stores local data to be reduced
     * @param recv_buf [out] the buffer to store result block containing @c recv_count elements of type @c dtype
     * @param recv_count number of elements of type @c dtype in receive block
     * @param reduction type of reduction operation to be applied
     * @param stream optional stream associated with the operation
     * @param deps optional vector of events that the execution should depend on
     * @param attr optional attributes to customize operation
     * @return @ref ccl::request_t object to track the progress of the operation
     */
    template <class buffer_object_type,
              class = typename std::enable_if<ccl::is_class_supported<buffer_object_type>()>::type>
    request_t reduce_scatter(const buffer_object_type& send_buf,
                             buffer_object_type& recv_buf,
                             size_t recv_count,
                             reduction reduction,
                             const stream_t& stream = stream_t(),
                             const std::vector<event_t>& deps = {},
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
