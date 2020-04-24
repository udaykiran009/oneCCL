#include "ccl.hpp"
#include "ccl_type_traits.hpp"
#include "common/comm/l0/communicator/process_group/process_a2a_communicator_impl.hpp"
#include "common/comm/l0/gpu_comm_attr.hpp"
#include "common/comm/l0/context/process_group_ctx.hpp"


using namespace ccl;

process_a2a_communicator::process_a2a_communicator(ccl::unified_device_type&& device,
                                                   size_t thread_idx,
                                                   size_t process_idx,
                                                   const ccl::shared_comm_device_attr_t& attr):
 base_t(std::move(device), thread_idx, process_idx, /*comm_attr, */attr)
{
}

void process_a2a_communicator::visit(ccl::gpu_comm_attr& comm_attr)
{
    ctx = comm_attr.get_process_context();

    //get rank & size
    auto topology = ctx->get_process_topology<base_t::topology_type()>(process_id, thread_id);
    this->initialize_comm_addr(get_device_path(), topology);
}

/***********************************************************************/
ALLREDUCE_EXPLICIT_INSTANTIATION(process_a2a_communicator, float);
