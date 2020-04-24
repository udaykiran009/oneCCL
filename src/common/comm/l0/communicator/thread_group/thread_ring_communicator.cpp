#include "ccl.hpp"
#include "ccl_type_traits.hpp"
#include "common/comm/l0/communicator/thread_group/thread_ring_communicator_impl.hpp"
#include "common/comm/l0/gpu_comm_attr.hpp"
#include "common/comm/l0/context/process_group_ctx.hpp"


using namespace ccl;

thread_device_group_ring_communicator::thread_device_group_ring_communicator(ccl::unified_device_type&& device,
                                                                             size_t thread_idx,
                                                                             size_t process_idx,
                                                                             const ccl::shared_comm_device_attr_t& attr):
 base_t(std::move(device), thread_idx, process_idx, /*comm_attr, */attr)
{
}

void thread_device_group_ring_communicator::visit(ccl::gpu_comm_attr& comm_attr)
{
    auto process_ctx = comm_attr.get_process_context();
    auto thread_ctx = process_ctx->get_thread_context(process_id);
    auto device_ctx = thread_ctx->get_device_group_ctx(thread_id);
    (void)device_ctx;

    ctx = thread_ctx;

    //get rank & size
    auto topology = ctx->get_thread_topology<base_t::topology_type()>(thread_id);
    this->initialize_comm_addr(get_device_path(), topology);
}

/*
size_t thread_device_group_ring_communicator::group_size() const
{
    return get_device_count<l0::ccl_gpu_comm>() +
              / * get_device_count<ccl_concurrent<SOURCE!!!>>() + Will add further* /
            get_device_count<l0::ccl_virtual_gpu_comm>();

}
*/

/***********************************************************************/
ALLREDUCE_EXPLICIT_INSTANTIATION(thread_device_group_ring_communicator, float);
