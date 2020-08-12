#pragma once
#include "common/comm/l0/communicator/typed_base_communicator.hpp"

namespace native
{
    struct thread_group_context;
}

class thread_device_group_a2a_communicator :
        public typed_base_communicator<thread_device_group_a2a_communicator,
                                       ccl::device_group_split_type::process,
                                       ccl::device_topology_type::a2a,
                                       ccl::gpu_communicator_traits>
{
public:
    using base_t = typed_base_communicator<thread_device_group_a2a_communicator,
                                           ccl::device_group_split_type::process,
                                           ccl::device_topology_type::a2a,
                                           ccl::gpu_communicator_traits>;

    thread_device_group_a2a_communicator(ccl::unified_device_type&& device,
                                         size_t thread_idx,
                                         size_t proces_idx,
                                         const ccl::device_comm_split_attr_t& attr);

    void visit(ccl::gpu_comm_attr& comm_attr) override;

    void barrier(const barrier_attr_t& attr,
                 ccl::stream::impl_value_t& op_stream,
                 const vector_class<event>& deps) override;

    DEVICE_COMM_IMPL_DECLARATION
    DEVICE_COMM_IMPL_CLASS_DECLARATION
    DEVICE_COMM_IMPL_SPARSE_DECLARATION
    DEVICE_COMM_IMPL_SPARSE_CLASS_DECLARATION

private:
    std::shared_ptr<native::thread_group_context> ctx;
};

//size_t group_size() const;
