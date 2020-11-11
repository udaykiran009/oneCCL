#pragma once
#include "common/comm/l0/communicator/typed_base_communicator.hpp"

namespace native {
struct process_group_context;
}

class process_a2a_communicator : public typed_base_communicator<process_a2a_communicator,
                                                                ccl::group_split_type::cluster,
                                                                ccl::device_topology_type::a2a,
                                                                ccl::gpu_communicator_traits> {
public:
    using base_t = typed_base_communicator<process_a2a_communicator,
                                           ccl::group_split_type::cluster,
                                           ccl::device_topology_type::a2a,
                                           ccl::gpu_communicator_traits>;

    using communication_devices_t = native::device_variant_t<native::ccl_gpu_comm,
                                                             native::ccl_virtual_gpu_comm,
                                                             native::ccl_ipc_source_gpu_comm<native::ccl_gpu_comm>,
                                                             native::ccl_ipc_source_gpu_comm<native::ccl_virtual_gpu_comm>
                                                             /*, TODO disabled t now
                                                             native::ccl_numa_proxy<native::ccl_gpu_comm>,
                                                             native::ccl_numa_proxy<native::ccl_virtual_gpu_comm>*/>;

    using coll_request_t = ccl::event;

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

    communication_devices_t& get_communication_device() {
        return communication_device;
    }

private:
    std::shared_ptr<native::process_group_context> ctx;
    communication_devices_t communication_device;
};
