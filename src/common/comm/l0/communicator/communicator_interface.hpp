#pragma once
#include "ccl.hpp"
#include "common/comm/l0/communicator/compiler_communicator_interface_dispatcher.hpp"

namespace native
{
    class ccl_device;
}

namespace ccl
{
class gpu_comm_attr;
struct communicator_interface : public communicator_interface_dispatcher
{
    using native_device_type     = typename ccl::unified_device_type::native_t;
    using native_device_type_ref = typename ccl::unified_device_type::native_reference_t;
    using native_device_type_ptr = typename ccl::unified_device_type::native_pointer_t;

    virtual ~communicator_interface() = default;

    virtual size_t rank() const = 0;
    virtual size_t size() const = 0;
    virtual ccl::device_index_type get_device_path() const = 0;
    virtual native_device_type_ref get_device() = 0;

    virtual const shared_comm_device_attr_t& get_attr() const = 0;
    virtual device_topology_type get_topology_type() const = 0;

    virtual bool is_ready() const = 0;
    virtual void visit(gpu_comm_attr& comm_attr) = 0;

    virtual device_communicator::coll_request_t allreduce(const float* send_buf,
                                                            float* recv_buf,
                                                            size_t count,
                                                            reduction reduction,
                                                            const coll_attr* attr,
                                                            ccl::stream::impl_t stream) = 0;
};
}
