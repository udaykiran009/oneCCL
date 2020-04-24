#pragma once
#include "common/comm/l0/communicator/compiler_communicator_interface_dispatcher.hpp"

#include "common/comm/l0/communicator/communicator_interface.hpp"
#include "common/comm/l0/communicator/device_group/device_ring_communicator.hpp"
#include "common/comm/l0/communicator/device_group/device_a2a_communicator.hpp"
#include "common/comm/l0/communicator/thread_group/thread_ring_communicator.hpp"
#include "common/comm/l0/communicator/thread_group/thread_a2a_communicator.hpp"
#include "common/comm/l0/communicator/process_group/process_ring_communicator.hpp"
#include "common/comm/l0/communicator/process_group/process_a2a_communicator.hpp"

namespace ccl
{

template <class DeviceType,
          typename std::enable_if<not std::is_same<typename std::remove_cv<DeviceType>::type,
                                                   ccl::device_index_type>::value,
                                  int>::type>
communicator_interface_ptr
communicator_interface_dispatcher::create_communicator_impl(const DeviceType& device,
                                                            size_t thread_idx,
                                                            size_t process_idx,
                                                            const ccl::shared_comm_device_attr_t& attr)
{
    static_assert(std::is_same<typename unified_device_type::device_t, DeviceType>::value, "Unsupported 'DeviceType'");

    return communicator_interface_dispatcher::create_communicator_from_unified_device(unified_device_type(device),
                                                                                      thread_idx,
                                                                                      process_idx,
                                                                                      attr);
}


template <class DeviceType,
          typename std::enable_if<std::is_same<typename std::remove_cv<DeviceType>::type,
                                               ccl::device_index_type>::value,
                                  int>::type>
communicator_interface_ptr
communicator_interface_dispatcher::create_communicator_impl(DeviceType device_id,
                                                            size_t thread_idx,
                                                            size_t process_idx,
                                                            const ccl::shared_comm_device_attr_t& attr)
{

#ifdef CCL_ENABLE_SYCL
    return communicator_interface_dispatcher::create_communicator_from_unified_device(unified_device_type(device_id, cl::sycl::info::device_type::gpu),
                                                                                                          thread_idx,
                                                                                                          process_idx,
                                                                                                          attr);
#else
    static_assert(std::is_same<typename unified_device_type::device_t, DeviceType>::value, "Unsupported 'DeviceType'");
    return communicator_interface_dispatcher::create_communicator_from_unified_device(unified_device_type(device_id),
                                                                                                          thread_idx,
                                                                                                          process_idx,
                                                                                                          attr);
#endif
}

communicator_interface_ptr
communicator_interface_dispatcher::create_communicator_from_unified_device(ccl::unified_device_type&& device_id,
                                                                           size_t thread_idx,
                                                                           size_t process_idx,
                                                                           const ccl::shared_comm_device_attr_t& attr)
{
    ccl::device_topology_class preferred_topology_class;
    ccl::device_topology_group preferred_topology_group;
    if (attr)
    {
        preferred_topology_class = attr->get_value<ccl_device_preferred_topology_class>();
        preferred_topology_group = attr->get_value<ccl_device_preferred_group>();
    }
    else
    {
        static const ccl_device_attr default_attr;
        preferred_topology_class = default_attr.get_value<ccl_device_preferred_topology_class>();
        preferred_topology_group = default_attr.get_value<ccl_device_preferred_group>();
    }

    //TODO check device_id or sycl device validity before communicator creation
    (void)preferred_topology_class;
    (void)preferred_topology_group;
    ccl::device_topology_type preferred_topology = ccl::device_topology_type::thread_group_ring;
    switch(preferred_topology)
    {
        case ccl::device_topology_type::device_group_ring:
            return communicator_interface_ptr(new device_group_ring_communicator(std::move(device_id), thread_idx, process_idx, attr));
        case ccl::device_topology_type::a2a_device_group:
            return communicator_interface_ptr(new device_group_a2a_communicator(std::move(device_id), thread_idx, process_idx, attr));

        case ccl::device_topology_type::thread_group_ring:
            return communicator_interface_ptr(new thread_device_group_ring_communicator(std::move(device_id), thread_idx, process_idx, attr));
        case ccl::device_topology_type::a2a_thread_group:
            return communicator_interface_ptr(new thread_device_group_a2a_communicator(std::move(device_id), thread_idx, process_idx, attr));

        case ccl::device_topology_type::allied_process_group_ring:
            return communicator_interface_ptr(new process_ring_communicator(std::move(device_id), thread_idx, process_idx, attr));
        case ccl::device_topology_type::a2a_allied_process_group:
            return communicator_interface_ptr(new process_a2a_communicator(std::move(device_id), thread_idx, process_idx,attr));

        default:
            throw ccl_error("Invalid color");
    }
    return std::unique_ptr<communicator_interface>();
}
}


#define COMMUNICATOR_INTERFACE_DISPATCHER_CLASS_EXPLICIT_INSTANTIATION(DeviceType)                      \
template ccl::communicator_interface_ptr                                                                \
ccl::communicator_interface_dispatcher::create_communicator_impl(const DeviceType& device,              \
                                                                 size_t thread_idx,                     \
                                                                 size_t process_idx,                    \
                                                                 const ccl::shared_comm_device_attr_t& attr);

#define COMMUNICATOR_INTERFACE_DISPATCHER_NON_CLASS_EXPLICIT_INSTANTIATION(DeviceType)                  \
template ccl::communicator_interface_ptr                                                                \
ccl::communicator_interface_dispatcher::create_communicator_impl(DeviceType device_id,                  \
                                                                 size_t thread_idx,                     \
                                                                 size_t process_idx,                    \
                                                                 const ccl::shared_comm_device_attr_t& attr);
