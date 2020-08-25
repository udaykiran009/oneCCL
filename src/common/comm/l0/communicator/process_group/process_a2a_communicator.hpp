#pragma once
#include "common/comm/l0/communicator/typed_base_communicator.hpp"

namespace native {
struct process_group_context;
}

class process_a2a_communicator
        : public typed_base_communicator<process_a2a_communicator,
                                         ccl::device_group_split_type::cluster,
                                         ccl::device_topology_type::a2a,
                                         ccl::gpu_communicator_traits> {
public:
    using base_t = typed_base_communicator<process_a2a_communicator,
                                           ccl::device_group_split_type::cluster,
                                           ccl::device_topology_type::a2a,
                                           ccl::gpu_communicator_traits>;

    process_a2a_communicator(ccl::unified_device_type&& device,
                             size_t thread_idx,
                             size_t proces_idx,
                             const ccl::device_comm_split_attr_t& attr);

    void visit(ccl::gpu_comm_attr& comm_attr) override;

    ccl::request_t barrier(const ccl::barrier_attr_t& attr,
                 ccl::stream::impl_value_t& op_stream,
                 const ccl::vector_class<ccl::event>& deps) override;

    DEVICE_COMM_IMPL_DECLARATION
    DEVICE_COMM_IMPL_CLASS_DECLARATION
    DEVICE_COMM_IMPL_SPARSE_DECLARATION
    DEVICE_COMM_IMPL_SPARSE_CLASS_DECLARATION

private:
    std::shared_ptr<native::process_group_context> ctx;
};
