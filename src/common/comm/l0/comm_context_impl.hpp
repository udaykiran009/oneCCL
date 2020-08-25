#include "ccl_types.hpp"
#include "ccl_kvs.hpp"
#include "common/log/log.hpp"
#include "common/comm/l0/gpu_comm_attr.hpp"
#include "common/comm/l0/comm_context.hpp"
#include "common/comm/comm_interface.hpp"
#if 1
    #include "../tests/unit/api/stubs/host_communicator.hpp"
#else
    #include "common/comm/host_communicator/host_communicator.hpp"
#endif

namespace ccl
{
/**
 *  Create communicator API:
 */
/*
CCL_API ccl::device_comm_split_attr_t ccl::comm_group::create_device_comm_split_attr()
{
    // TODO
    const auto& host_comm = pimpl->get_host_communicator();
    return ccl::device_comm_split_attr_t{new ccl::ccl_device_attr(*(host_comm->get_comm_split_attr()))};
}
*/
/*
 *  Single device communicator creation
 */
template <class DeviceType,
          typename std::enable_if<std::is_class<typename std::remove_cv<DeviceType>::type>::value,
                                      int>::type>
CCL_API ccl::device_communicator ccl::comm_group::create_communicator(const DeviceType& device,
                                                                     ccl::device_comm_split_attr_t attr/* = comm_device_attr_t()*/)
{
    LOG_TRACE("Create communicator from device");
    ccl::communicator_interface_ptr impl =
            ccl::communicator_interface::create_communicator_impl(device,
                                                                  pimpl->thread_id,
                                                                  pimpl->get_host_communicator()->rank(),
                                                                  attr);
    // registering device in group - is non blocking operation, until it is not the last device
    pimpl->sync_register_communicator(impl.get());
    return device_communicator(std::move(impl));
}

template <class DeviceType,
          typename std::enable_if<not std::is_class<typename std::remove_cv<DeviceType>::type>::value,
                                      int>::type>
CCL_API ccl::device_communicator ccl::comm_group::create_communicator(DeviceType device_id,
                                                                    ccl::device_comm_split_attr_t attr/* = nullptr*/)
{
    LOG_TRACE("Create communicator from id: ", device_id);

    ccl::communicator_interface_ptr impl = ccl::communicator_interface::create_communicator_impl(device_id,
                                                                                                 pimpl->thread_id,
                                                                                                 pimpl->get_host_communicator()->rank(),
                                                                                                 attr);
    // registering device in group - is non blocking operation, until it is not the last device
    pimpl->sync_register_communicator(impl.get());
    return ccl::device_communicator(std::move(impl));
}

/**
 *  Multiple device communicators creation vectorized API implementation
 */
template<class InputIt>
CCL_API std::vector<ccl::device_communicator> ccl::comm_group::create_communicators(InputIt first, InputIt last,
                                                                                   ccl::device_comm_split_attr_t attr/* = nullptr*/)
{

    using iterator_value_type = typename std::iterator_traits<InputIt>::value_type;
/*
    using expected_value_type = typename unified_device_type::device_t;
    static_assert(std::is_same<iterator_value_type, expected_value_type>::value,
                  "Not valid InputIt in create_communicators");
*/
    size_t indices_count = std::distance(first, last);
    LOG_TRACE("Create device communicators from index iterators type, count: ", indices_count);

    std::vector<ccl::device_communicator> comms;
    comms.reserve(indices_count);
    std::transform(first, last, std::back_inserter(comms), [this, attr](const iterator_value_type& device_id)
    {
        return create_communicator(device_id, attr);
    });
    return comms;
}

template<template<class...> class Container, class Type>
CCL_API std::vector<ccl::device_communicator> ccl::comm_group::create_communicators(const Container<Type>& device_ids,
                                                                                   ccl::device_comm_split_attr_t attr/* = nullptr*/)
{
    //static_assert(std::is_same<Type, ccl::device_index_type>::value, "Invalid Type in create_communicators");
    LOG_TRACE("Create device communicators from index type, count: ", device_ids.size(),
              ". Redirect to iterators version");
    return create_communicators(device_ids.begin(), device_ids.end(), attr);
}
/*
CCL_API ccl::comm_group::device_context_native_const_reference_t ccl::comm_group::get_context() const
{
    //TODO use PIMPL as context provider
    static unified_device_context_type context;
    return context.get();
}
*/

/***********************************************************************
#define COMM_CREATOR_INDEXED_INSTANTIATION_CONTAINER(type)                                         \
template std::vector<ccl::communicator_t>                                                          \
CCL_API ccl::comm_group::create_communicators(const type& device_ids,                              \
                                              ccl::device_comm_split_attr_t attr);
*/
}
