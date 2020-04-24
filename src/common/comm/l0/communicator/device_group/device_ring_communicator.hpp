#pragma once
#include "common/comm/l0/communicator/typed_base_communicator.hpp"

namespace native
{
    class device_group_context;
}

class device_group_ring_communicator :
        public typed_base_communicator<ccl::device_topology_type::device_group_ring>
{
public:
    using base_t = typed_base_communicator<ccl::device_topology_type::device_group_ring>;

    device_group_ring_communicator(ccl::unified_device_type&& device,
                                   size_t thread_idx,
                                   size_t proces_idx,
                                   const ccl::shared_comm_device_attr_t& attr);

    void visit(ccl::gpu_comm_attr& comm_attr) override;

    ccl::device_communicator::coll_request_t allreduce(const float* send_buf,
                                                    float* recv_buf,
                                                    size_t count,
                                                    ccl::reduction reduction,
                                                    const ccl::coll_attr* attr,
                                                    ccl::stream::impl_t stream) override
    {
        return allreduce_impl(send_buf, recv_buf, count, reduction, attr, stream);
    }

private:

    template<class buffer_type>
    ccl::device_communicator::coll_request_t allreduce_impl(const buffer_type* send_buf,
                                                         buffer_type* recv_buf,
                                                         size_t count,
                                                         ccl::reduction reduction,
                                                         const ccl::coll_attr* attr,
                                                         ccl::stream::impl_t& stream);

    std::shared_ptr<native::device_group_context> ctx;
};
