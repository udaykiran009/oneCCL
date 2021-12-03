#pragma once

#include <cstddef>
#include <memory>

#include "oneapi/ccl/types.hpp"
#include "communicator_traits.hpp"
#include "atl/atl_base_comm.hpp"

namespace native {
struct ccl_device;
}
namespace ccl {
namespace v1 {
class comm_split_attr;
}

struct communicator_interface;

using communicator_interface_ptr = std::shared_ptr<communicator_interface>;

struct communicator_interface_dispatcher {
    using device_t = ccl::device;
    using context_t = ccl::context;

    using device_ptr_t = std::shared_ptr<ccl::device>;
    using context_ptr_t = std::shared_ptr<ccl::context>;

    virtual ~communicator_interface_dispatcher() = default;

    virtual device_ptr_t get_device() const = 0;
    virtual context_ptr_t get_context() const = 0;

    static communicator_interface_ptr create_communicator_impl();
    static communicator_interface_ptr create_communicator_impl(const size_t size,
                                                               shared_ptr_class<ikvs_wrapper> kvs);
    static communicator_interface_ptr create_communicator_impl(const size_t size,
                                                               const int rank,
                                                               shared_ptr_class<ikvs_wrapper> kvs);
    static communicator_interface_ptr create_communicator_impl(const size_t size,
                                                               const int rank,
                                                               device_t ccl_device,
                                                               context_t ccl_context,
                                                               shared_ptr_class<ikvs_wrapper> kvs);
};

} // namespace ccl
