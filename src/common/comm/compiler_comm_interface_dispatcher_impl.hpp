#pragma once
#include "common/comm/compiler_comm_interface_dispatcher.hpp"

#include "common/comm/comm_interface.hpp"
#include "unified_device_impl.hpp"

#include "ccl_types_policy.hpp"
#include "ccl_comm_split_attr_ids.hpp"
#include "ccl_comm_split_attr_ids_traits.hpp"
#include "ccl_comm_split_attr.hpp"

#include "common/comm/comm_split_common_attr.hpp"
#include "comm_split_attr_impl.hpp"

#ifdef MULTI_GPU_SUPPORT
#include "common/comm/l0/communicator/device_group/device_ring_communicator.hpp"
#include "common/comm/l0/communicator/device_group/device_a2a_communicator.hpp"
#include "common/comm/l0/communicator/thread_group/thread_ring_communicator.hpp"
#include "common/comm/l0/communicator/thread_group/thread_a2a_communicator.hpp"
#include "common/comm/l0/communicator/process_group/process_ring_communicator.hpp"
#include "common/comm/l0/communicator/process_group/process_a2a_communicator.hpp"
#include "supported_topologies.hpp"

#endif

#include "common/comm/single_device_communicator/single_device_communicator.hpp"


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
                                                            const ccl::device_comm_split_attr_t& attr)
{
    static_assert(std::is_same<typename unified_device_type::handle_t, DeviceType>::value, "Unsupported 'DeviceType'");

    return communicator_interface_dispatcher::create_communicator_from_unified_device(
                        unified_device_type(device),
                        thread_idx, process_idx, attr);
}


template <class DeviceType,
          typename std::enable_if<std::is_same<typename std::remove_cv<DeviceType>::type,
                                               ccl::device_index_type>::value,
                                  int>::type>
communicator_interface_ptr
communicator_interface_dispatcher::create_communicator_impl(DeviceType device_id,
                                                            size_t thread_idx,
                                                            size_t process_idx,
                                                            const ccl::device_comm_split_attr_t& attr)
{

#ifdef CCL_ENABLE_SYCL
    return communicator_interface_dispatcher::create_communicator_from_unified_device(
                            unified_device_type(device_id, cl::sycl::info::device_type::gpu),
                            thread_idx, process_idx, attr);
#else
    static_assert(std::is_same<typename unified_device_type::handle_t, DeviceType>::value, "Unsupported 'DeviceType'");
    return communicator_interface_dispatcher::create_communicator_from_unified_device(
                            unified_device_type(device_id),
                            thread_idx, process_idx, attr);
#endif
}

communicator_interface_ptr
communicator_interface_dispatcher::create_communicator_from_unified_device(ccl::unified_device_type&& device_id,
                                                                           size_t thread_idx,
                                                                           size_t process_idx,
                                                                           const ccl::device_comm_split_attr_t& attr)
{
    // TODO ring by default at now. Choose preferred a2a if availbale
    ccl::device_topology_type preferred_topology_class = ccl::device_topology_type::ring;
    ccl::device_group_split_type preferred_topology_group = ccl::device_group_split_type::cluster;

    // read comm split attributes
    if (attr.is_valid<ccl::ccl_comm_split_attributes::group>())
    {
        preferred_topology_group = attr.get<ccl::ccl_comm_split_attributes::group>();
        if(attr.is_valid<ccl::ccl_comm_split_attributes::color>())
        {
            throw ccl_error(std::string("Invalid `device_comm_split_attr_t`: both `color` and `group` set. Only one is supported"));
        }
    }
    else if (attr.is_valid<ccl::ccl_comm_split_attributes::color>())
    {
        throw ccl_error(std::string(__FUNCTION__) + " - not implemented for 'color'");
    }

    //TODO check device_id or sycl device validity before communicator creation
    switch(preferred_topology_class)
    {
        case device_topology_type::ring:
        {
            switch(preferred_topology_group)
            {
                case ccl::device_group_split_type::undetermined:
                {
                    auto comm_impl =
                                    new single_device_communicator(std::move(device_id),
                                                                       thread_idx, process_idx,
                                                                       attr);

                    ccl::global_data& data = ccl::global_data::get();
                    auto comm = std::shared_ptr<ccl_comm>(
                            new ccl_comm(thread_idx, process_idx, data.comm_ids->acquire()));
                    comm_impl->set_ccl_comm(std::move(comm));
                    return communicator_interface_ptr(comm_impl);
                }
                case ccl::device_group_split_type::thread:
                    return communicator_interface_ptr(
                                    new device_group_ring_communicator(std::move(device_id),
                                                                       thread_idx, process_idx,
                                                                       attr));
                case ccl::device_group_split_type::process:
                    return communicator_interface_ptr(
                                    new thread_device_group_ring_communicator(std::move(device_id),
                                                                              thread_idx, process_idx,
                                                                              attr));
                case ccl::device_group_split_type::cluster:
                    return communicator_interface_ptr(
                                    new process_ring_communicator(std::move(device_id),
                                                                  thread_idx, process_idx,
                                                                   attr));
                default:
                    throw ccl_error(std::string("Invalid `device_comm_split_attr_t` value for `ccl_device_preferred_group`: ") +
                            ::to_string(preferred_topology_group));
            }
            break;
        }
        case device_topology_type::a2a:
        {
            switch(preferred_topology_group)
            {
                case ccl::device_group_split_type::undetermined:
                    return communicator_interface_ptr(
                                    new single_device_communicator(std::move(device_id),
                                                                       thread_idx, process_idx,
                                                                       attr));
                case ccl::device_group_split_type::thread:
                    return communicator_interface_ptr(
                                    new device_group_a2a_communicator(std::move(device_id),
                                                                      thread_idx, process_idx, attr));
                case ccl::device_group_split_type::process:
                    return communicator_interface_ptr(new thread_device_group_a2a_communicator(
                                                                      std::move(device_id),
                                                                      thread_idx, process_idx, attr));
                case ccl::device_group_split_type::cluster:
                    return communicator_interface_ptr(new process_a2a_communicator(std::move(device_id),
                                                                      thread_idx, process_idx, attr));
                default:
                    throw ccl_error(std::string("Invalid `device_comm_split_attr_t` value for `ccl_device_preferred_group`: ") +
                            ::to_string(preferred_topology_group));
            }
            break;
        }
        case device_topology_type::undetermined:
        {
            auto comm_impl =
                                    new single_device_communicator(std::move(device_id),
                                                                       thread_idx, process_idx,
                                                                       attr);

            ccl::global_data& data = ccl::global_data::get();
            auto comm = std::shared_ptr<ccl_comm>(
                        new ccl_comm(thread_idx, process_idx, data.comm_ids->acquire()));
            comm_impl->set_ccl_comm(std::move(comm));
            return communicator_interface_ptr(comm_impl);
        }
        default:
        {
            throw ccl_error(std::string("Invalid `device_comm_split_attr_t` value for `ccl_device_preferred_topology_class`: ") +
                            ::to_string(preferred_topology_class));
        }
    }

    return std::unique_ptr<communicator_interface>();
}


#define COMMUNICATOR_INTERFACE_DISPATCHER_CLASS_EXPLICIT_INSTANTIATION(DeviceType)                      \
template ccl::communicator_interface_ptr                                                                \
ccl::communicator_interface_dispatcher::create_communicator_impl(const DeviceType& device,              \
                                                                 size_t thread_idx,                     \
                                                                 size_t process_idx,                    \
                                                                 const ccl::device_comm_split_attr_t& attr);

#define COMMUNICATOR_INTERFACE_DISPATCHER_NON_CLASS_EXPLICIT_INSTANTIATION(DeviceType)                  \
template ccl::communicator_interface_ptr                                                                \
ccl::communicator_interface_dispatcher::create_communicator_impl(DeviceType device_id,                  \
                                                                 size_t thread_idx,                     \
                                                                 size_t process_idx,                    \
                                                                 const ccl::device_comm_split_attr_t& attr);

}
