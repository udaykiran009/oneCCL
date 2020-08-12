#pragma once
#include <cstddef>
#include <memory>

#include "ccl_types.hpp"
#include "supported_topologies.hpp"
#include "communicator_traits.hpp"

#ifdef MULTI_GPU_SUPPORT
class gpu_comm_attr;
#endif

namespace native
{
    struct ccl_device;
}
namespace ccl
{

struct communicator_interface;
class device_comm_split_attr_t;

using communicator_interface_ptr = std::shared_ptr<communicator_interface>;

struct communicator_interface_dispatcher
{
    using device_t = typename ccl::unified_device_type::ccl_native_t;
    using context_t = typename ccl::unified_device_context_type::ccl_native_t;

    virtual void visit(gpu_comm_attr& comm_attr) = 0;
    virtual ccl::device_index_type get_device_path() const = 0;
    virtual device_t get_device() = 0;
    virtual context_t get_context() = 0;
    virtual const device_comm_split_attr_t& get_comm_split_attr() const = 0;
    virtual device_group_split_type get_topology_type() const = 0;
    virtual device_topology_type get_topology_class() const = 0;

    // create communicator for device & cpu types (from device class)
    template <class DeviceType,
              typename std::enable_if<not std::is_same<typename std::remove_cv<DeviceType>::type,
                                                       device_index_type>::value,
                                      int>::type = 0>
    static communicator_interface_ptr create_communicator_impl(const DeviceType& device,
                                                               size_t thread_idx,
                                                               size_t process_idx,
                                                               const device_comm_split_attr_t& attr);

    // create communicator for device & cpu types (from device index)
    template <class DeviceType,
              typename std::enable_if<std::is_same<typename std::remove_cv<DeviceType>::type,
                                                   device_index_type>::value,
                                      int>::type  = 0>
    static communicator_interface_ptr create_communicator_impl(DeviceType device_id,
                                                               size_t thread_idx,
                                                               size_t process_idx,
                                                               const device_comm_split_attr_t& attr);
private:

    static communicator_interface_ptr create_communicator_from_unified_device(unified_device_type&& device_id,
                                                                              size_t thread_idx,
                                                                              size_t process_idx,
                                                                              const device_comm_split_attr_t& attr);

};
}
