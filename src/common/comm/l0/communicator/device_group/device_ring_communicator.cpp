#include "ccl.hpp"
#include "ccl_type_traits.hpp"
#include "common/comm/l0/communicator/device_group/device_ring_communicator_impl.hpp"
#include "common/comm/l0/gpu_comm_attr.hpp"
#include "common/comm/l0/context/thread_group_ctx.hpp"
#include "common/comm/l0/context/process_group_ctx.hpp"

using namespace ccl;

device_group_ring_communicator::device_group_ring_communicator(ccl::unified_device_type&& device,
                                                               size_t thread_idx,
                                                               size_t process_idx,
                                                               const ccl::shared_comm_device_attr_t& attr):
 base_t(std::move(device), thread_idx, process_idx/*, comm_attr*/, attr)
{
}

void device_group_ring_communicator::visit(ccl::gpu_comm_attr& comm_attr)
{
    auto process_ctx = comm_attr.get_process_context();
    auto thread_ctx = process_ctx->get_thread_context(process_id);
    auto device_ctx = thread_ctx->get_device_group_ctx(thread_id);

    ctx = device_ctx;

    //get rank & size

    this->initialize_comm_addr(get_device_path(),
                               ctx->get_group_topology<base_t::topology_type()>());
}

/***********************************************************************/
//COLL_EXPLICIT_INSTANTIATION(char);
//COLL_EXPLICIT_INSTANTIATION(int);
//COLL_EXPLICIT_INSTANTIATION(int64_t);
//COLL_EXPLICIT_INSTANTIATION(uint64_t);
ALLREDUCE_EXPLICIT_INSTANTIATION(device_group_ring_communicator, float);
//COLL_EXPLICIT_INSTANTIATION(double);
