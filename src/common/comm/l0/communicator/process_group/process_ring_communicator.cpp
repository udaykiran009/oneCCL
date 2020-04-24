#include "ccl.hpp"
#include "common/comm/l0/communicator/process_group/process_ring_communicator_impl.hpp"

#include "common/comm/l0/gpu_comm_attr.hpp"

using namespace ccl;


process_ring_communicator::process_ring_communicator(ccl::unified_device_type&& device,
                                                     size_t thread_idx,
                                                     size_t process_idx,
                                                     const ccl::shared_comm_device_attr_t& attr):
 base_t(std::move(device), thread_idx, process_idx, /*comm_attr,*/ attr)
{
}

void process_ring_communicator::visit(ccl::gpu_comm_attr& comm_attr)
{
    ctx = comm_attr.get_process_context();

    //get rank & size
    auto topology = ctx->get_process_topology<base_t::topology_type()>(process_id, thread_id);
    this->initialize_comm_addr(get_device_path(), topology);
}
/*
size_t process_ring_communicator::group_size() const
{
    return get_device_count<l0::ccl_gpu_comm>() +
           get_device_count<l0::ccl_ipc_source_gpu_comm<l0::ccl_gpu_comm>>() +
        / * get_device_count<ccl_ipc_gpu_comm>() + do no participate in group  communication* /
           get_device_count<l0::ccl_virtual_gpu_comm>();

}
*/

/***********************************************************************/
ALLREDUCE_EXPLICIT_INSTANTIATION(process_ring_communicator, float);
