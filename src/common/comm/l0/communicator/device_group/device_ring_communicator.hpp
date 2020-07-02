#pragma once
#include "common/comm/l0/communicator/typed_base_communicator.hpp"

namespace native
{
    struct device_group_context;
}

class device_group_ring_communicator :
        public typed_base_communicator<device_group_ring_communicator,
                                       ccl::device_group_split_type::thread,
                                       ccl::device_topology_type::ring,
                                       ccl::gpu_communicator_traits>
{
public:
    using base_t = typed_base_communicator<device_group_ring_communicator,
                                           ccl::device_group_split_type::thread,
                                           ccl::device_topology_type::ring,
                                           ccl::gpu_communicator_traits>;

    device_group_ring_communicator(ccl::unified_device_type&& device,
                                   size_t thread_idx,
                                   size_t proces_idx,
                                   const ccl::device_comm_attr_t& attr);

    void visit(ccl::gpu_comm_attr& comm_attr) override;

    void barrier(ccl::stream::impl_t& stream) override;

    COMM_IMPL_DECLARATION
    COMM_IMPL_CLASS_DECLARATION
    COMM_IMPL_SPARSE_DECLARATION
    COMM_IMPL_SPARSE_CLASS_DECLARATION

private:
    std::shared_ptr<native::device_group_context> ctx;
};
