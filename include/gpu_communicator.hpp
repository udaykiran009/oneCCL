#pragma once

#ifdef MULTI_GPU_SUPPORT
#ifndef CCL_PRODUCT_FULL
    #error "Do not include this file directly. Please include 'ccl_types.hpp'"
#endif

namespace ccl
{
struct gpu_comm_attr;
struct communicator_interface;

class comm_group
{
public:
    friend class environment;

    using device_context_native_reference_t = typename unified_device_context_type::native_reference_t;
    using device_context_native_const_reference_t = typename unified_device_context_type::native_const_reference_t;
    /**
     * Device Communicator creation API: single communicator creation, based on @device
     */
    template <class DeviceType,
              typename std::enable_if<std::is_class<typename std::remove_cv<DeviceType>::type>::value,
                                      int>::type = 0>
    communicator_t create_communicator(const DeviceType& device,
                                       device_comm_attr_t attr = device_comm_attr_t());

    /**
     * Created @device_comm_attr_t, which used to create device_communicators from @comm_group_t
     */
    device_comm_attr_t create_device_comm_attr();

    /**
     * Device Communicator creation API: single communicator creation, based on index @device_id
     */
    template <class DeviceType,
              typename std::enable_if<not std::is_class<typename std::remove_cv<DeviceType>::type>::value,
                                      int>::type = 0>
    communicator_t create_communicator(DeviceType device_id,
                                       device_comm_attr_t attr = device_comm_attr_t());

    /**
     * Device Communicator creation vectorized API:
     * multiple communicator creation, based on devices of @Type, packed into container @Container
     */
    template<template<class...> class Container, class Type>
    std::vector<communicator_t> create_communicators(const Container<Type>& device_ids,
                                                     device_comm_attr_t attr = device_comm_attr_t());

    /**
     * Device Communicator creation vectorized API:
     * multiple communicator creation, based on devices iterator @InputIt
     */
    template<class InputIt>
    std::vector<communicator_t> create_communicators(InputIt first, InputIt last,
                                                     device_comm_attr_t attr = device_comm_attr_t());

    /**
     * Return device context allocated during group creation
     */
    device_context_native_const_reference_t get_context() const;

private:
    comm_group(ccl::shared_communicator_t comm, size_t current_device_group_size, size_t process_device_group_size);
    std::unique_ptr<gpu_comm_attr> pimpl;
};

#if DEPRECATED
class communicator final
{
public:

    /**
     * Type allows to get underlying device type,
     * which was used as communicator construction argument
     */
    using device_native_reference_t = typename unified_device_type::native_reference_t;

    ~communicator();

    /**
     * Type allows to operate request interface in RAII manner
     */
    using coll_request_t = std::unique_ptr<request>;

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
     * Retrieves underlying device, which was used as communicator constrution argument
     */
    device_native_reference_t get_device();

    /**
     * Retrieves status of wiring-up progress of communicators in group
     * After all expected communicators are created in parent comm_group,
     * then wiring-up phase is automatically executed
     * and all communicator object will go in ready status
     */
    bool is_ready() const;

    /**
     * Retrieves logically determined devices topology based on hardware preferred
     * devices topology. Can be overriden during communicator creation phase
     */
    ccl::device_group_split_type get_topology_type() const;

    /**
     * Type safety version:
     * Reduces @c buf on all process in the communicator and stores result in @c recv_buf
     * on each process
     * @param send_buf the buffer with @c count elements of @c buffer_type that stores local data to be reduced
     * @param recv_buf [out] - the buffer to store reduced result , must have the same dimension
     * as @c buf.
     * @param count number of elements of type @c buffer_type in @c buf
     * @param reduction type of reduction operation to be applied
     * @param attr optional attributes that customize operation
     * @return @ref ccl::communicator::coll_request_t object that can be used to track the progress of the operation
     */
    template<class buffer_type,
             class = typename std::enable_if<ccl::is_native_type_supported<buffer_type>()>::type>
    coll_request_t allreduce(const buffer_type* send_buf,
                             buffer_type* recv_buf,
                             size_t count,
                             ccl::reduction reduction,
                             const ccl::coll_attr* attr,
                             const ccl::stream_t& stream);

private:
    friend class environment;
    friend class comm_group;

    explicit communicator(std::shared_ptr<communicator_interface> impl);

    /**
     * Holds specific-implementation details of communicator
     */
    std::shared_ptr<communicator_interface> pimpl;

};
#endif //DEPRECATED
}

#endif //MULTI_GPU_SUPPORT
