#pragma once

#ifdef MULTI_GPU_SUPPORT
#ifndef CCL_PRODUCT_FULL
    #error "Do not include this file directly. Please include 'ccl_types.hpp'"
#endif

namespace ccl
{
class gpu_comm_attr;
class communicator_interface;
class device_attr_impl;

/**
 * Used to create device communicator with specific attributes,
 * which is differ from just 'communicator' attributes in common way
 */
class ccl_device_attr : public ccl_comm_device_attr_t::device_core_attr
{
public:
    friend class environment;
    friend class communicator_interface_dispatcher;
    ~ccl_device_attr() noexcept;

    /**
     * Set specific value for attribute by @attrId.
     * Previous attibute value would be returned
     */
    template<ccl_device_attributes attrId,
             class Value,
             class = typename std::enable_if<std::is_same<typename ccl_device_attributes_traits<attrId>::type, Value>::value>::type>
    Value set_value(Value&& v);

    /**
     * Get specific attribute value by @attrId
     */
    template<ccl_device_attributes attrId>
    const typename ccl_device_attributes_traits<attrId>::type& get_value() const;

private:
    ccl_device_attr();
    std::unique_ptr<device_attr_impl> pimpl;
};


class comm_group
{
public:
    friend class environment;

    using device_context_native_reference_t = typename unified_device_context_type::native_reference_t;
    using device_context_native_const_reference_t = typename unified_device_context_type::native_const_reference_t;
    /**
     * Communicator creation API: single communicator creation, based on @device
     */
    template <class DeviceType,
              typename std::enable_if<std::is_class<typename std::remove_cv<DeviceType>::type>::value,
                                      int>::type = 0>
    device_communicator_t create_communicator(const DeviceType& device,
                                           shared_comm_device_attr_t attr = shared_comm_device_attr_t());

    /**
     * Communicator creation API: single communicator creation, based on index @device_id
     */
    template <class DeviceType,
              typename std::enable_if<not std::is_class<typename std::remove_cv<DeviceType>::type>::value,
                                      int>::type = 0>
    device_communicator_t create_communicator(DeviceType device_id,
                                           shared_comm_device_attr_t attr = shared_comm_device_attr_t());

    /**
     * Communicator creation API: multiple communicator creation, based on devices of @Type, packed into container @Container
     */
    template<template<class...> class Container, class Type>
    std::vector<device_communicator_t> create_communicators(const Container<Type>& device_ids,
                                                         shared_comm_device_attr_t attr = shared_comm_device_attr_t());

    /**
     * Communicator creation API: multiple communicator creation, based on devices iterator @InputIt
     */
    template<class InputIt>
    std::vector<device_communicator_t> create_communicators(InputIt first, InputIt last,
                                                         shared_comm_device_attr_t attr = shared_comm_device_attr_t());

    /**
     * Return device context allocated during group creation
     */
    device_context_native_const_reference_t get_context() const;

private:
    comm_group(ccl::shared_communicator_t comm, size_t current_device_group_size, size_t process_device_group_size);
    std::unique_ptr<gpu_comm_attr> pimpl;
};


class device_communicator final
{
public:

    /**
     * Type allows to get underlying device type,
     * which was used as communicator construction argument
     */
    using device_native_reference_t = typename unified_device_type::native_reference_t;

    ~device_communicator();

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
    ccl::device_topology_type get_topology_type() const;

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

    explicit device_communicator(std::shared_ptr<communicator_interface> impl);

    /**
     * Holds specific-implementation details of communicator
     */
    std::shared_ptr<communicator_interface> pimpl;

};

}

#endif //MULTI_GPU_SUPPORT
