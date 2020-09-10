#pragma once
#include "common/comm/l0/communicator/typed_base_communicator.hpp"
#include "common/comm/comm.hpp"

namespace native
{
    struct device_group_context;
}

class single_device_communicator :
        public typed_base_communicator<single_device_communicator,
                                       ccl::device_group_split_type::undetermined,
                                       ccl::device_topology_type::undetermined,
                                       ccl::gpu_communicator_traits>
{
public:
    using base_t = typed_base_communicator<single_device_communicator,
                                           ccl::device_group_split_type::undetermined,
                                           ccl::device_topology_type::undetermined,
                                           ccl::gpu_communicator_traits>;

    single_device_communicator(ccl::unified_device_type&& device,
                                   size_t thread_idx,
                                   size_t proces_idx,
                                   const ccl::device_comm_split_attr& attr);

    void visit(ccl::gpu_comm_attr& comm_attr) override;

    ccl::request_t barrier(ccl::stream::impl_value_t& op_stream,
                 const ccl::barrier_attr& attr,
                 const ccl::vector_class<ccl::event>& deps) override;

    DEVICE_COMM_IMPL_DECLARATION
    DEVICE_COMM_IMPL_CLASS_DECLARATION
    DEVICE_COMM_IMPL_SPARSE_DECLARATION
    DEVICE_COMM_IMPL_SPARSE_CLASS_DECLARATION

    void set_ccl_comm(std::shared_ptr<ccl_comm> imp);
private:
    std::shared_ptr<ccl_comm> comm_impl;
};
