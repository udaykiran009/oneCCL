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

    void barrier(ccl::stream::impl_t& stream) override;

    COMM_IMPL_DECLARATION
    COMM_IMPL_CLASS_DECLARATION
    COMM_IMPL_SPARSE_DECLARATION
    COMM_IMPL_SPARSE_CLASS_DECLARATION

private:
    std::shared_ptr<native::process_group_context> ctx;
};
