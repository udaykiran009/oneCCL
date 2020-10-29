#pragma once
#include "common/comm/l0/communicator/typed_base_communicator.hpp"

namespace native {
struct process_group_context;
}

class process_a2a_communicator
        : public typed_base_communicator<process_a2a_communicator,
                                         ccl::group_split_type::cluster,
                                         ccl::device_topology_type::a2a,
                                         ccl::gpu_communicator_traits> {
public:
    using base_t = typed_base_communicator<process_a2a_communicator,
                                           ccl::group_split_type::cluster,
                                           ccl::device_topology_type::a2a,
                                           ccl::gpu_communicator_traits>;

    process_a2a_communicator(ccl::unified_device_type&& device,
                             ccl::unified_context_type&& ctx,
                             size_t thread_idx,
                             size_t proces_idx,
                             const ccl::comm_split_attr& attr);

    void visit(ccl::gpu_comm_attr& comm_attr) override;

    ccl::event barrier(const ccl::stream::impl_value_t& stream,
                           const ccl::barrier_attr& attr,
                           const ccl::vector_class<ccl::event>& deps) override;

    COMM_IMPL_DECLARATION
    COMM_IMPL_CLASS_DECLARATION
    COMM_IMPL_SPARSE_DECLARATION
    COMM_IMPL_SPARSE_CLASS_DECLARATION

private:
    std::shared_ptr<native::process_group_context> ctx;
};
