#pragma once
#include "common/comm/l0/communicator/typed_base_communicator.hpp"

namespace native {
struct thread_group_context;
}

class thread_device_group_ring_communicator
        : public typed_base_communicator<thread_device_group_ring_communicator,
                                         ccl::device_group_split_type::process,
                                         ccl::device_topology_type::ring,
                                         ccl::gpu_communicator_traits> {
public:
    using base_t = typed_base_communicator<thread_device_group_ring_communicator,
                                           ccl::device_group_split_type::process,
                                           ccl::device_topology_type::ring,
                                           ccl::gpu_communicator_traits>;

    thread_device_group_ring_communicator(ccl::unified_device_type&& device,
                                          size_t thread_idx,
                                          size_t process_idx,
                                          const ccl::device_comm_split_attr& attr);

    void visit(ccl::gpu_comm_attr& comm_attr) override;

    ccl::request_t barrier(ccl::stream::impl_value_t& stream, const ccl::barrier_attr& attr,
                 const ccl::vector_class<ccl::event>& deps) override;

    DEVICE_COMM_IMPL_DECLARATION
    DEVICE_COMM_IMPL_CLASS_DECLARATION
    DEVICE_COMM_IMPL_SPARSE_DECLARATION
    DEVICE_COMM_IMPL_SPARSE_CLASS_DECLARATION

private:
    std::shared_ptr<native::thread_group_context> ctx;
};
