#pragma once
#include <cstddef>
#include <memory>


#include "ccl.hpp"

namespace native
{
    struct ccl_device;
}
namespace ccl
{

struct communicator_interface;
using communicator_interface_ptr = std::shared_ptr<communicator_interface>;

struct communicator_interface_dispatcher
{
    template <class DeviceType,
              typename std::enable_if<not std::is_same<typename std::remove_cv<DeviceType>::type,
                                                   ccl::device_index_type>::value,
                                      int>::type = 0>
    static communicator_interface_ptr create_communicator_impl(const DeviceType& device,
                                                               size_t thread_idx,
                                                               size_t process_idx,
                                                               const ccl::shared_comm_device_attr_t& attr);

    template <class DeviceType,
              typename std::enable_if<std::is_same<typename std::remove_cv<DeviceType>::type,
                                               ccl::device_index_type>::value,
                                      int>::type  = 0>
    static communicator_interface_ptr create_communicator_impl(DeviceType device_id,
                                                               size_t thread_idx,
                                                               size_t process_idx,
                                                               const ccl::shared_comm_device_attr_t& attr);
private:
    static communicator_interface_ptr create_communicator_from_unified_device(ccl::unified_device_type&& device_id,
                                                                              size_t thread_idx,
                                                                              size_t process_idx,
                                                                              const ccl::shared_comm_device_attr_t& attr);
};
}
