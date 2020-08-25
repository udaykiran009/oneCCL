#include <fstream>
#include <vector>
#include <sstream>
#include <iterator>
#include <set>
#include <unistd.h>
#include <limits.h>
#include <gnu/libc-version.h>

#include "common/comm/l0/context/process_group_ctx.hpp"
#include "common/comm/l0/gpu_comm_attr.hpp"
#if 1
    #include "../tests/unit/api/stubs/host_communicator.hpp"
#else
    #include "common/comm/host_communicator/host_communicator.hpp"
#endif
namespace native
{
struct allied_process_group_scheduler {};
struct device_storage{};

process_group_context::process_group_context(std::shared_ptr<host_communicator> comm) :
    ccl_communicator(comm)
{}

process_group_context::~process_group_context()
{
}

bool process_group_context::sync_barrier(const ccl::device_mask_t& thread_device_mask,
                                              ccl::context_comm_addr& comm_addr)
{
    return true;
}

bool process_group_context::sync_barrier(const ccl::device_indices_t& thread_device_indices,
                                              ccl::context_comm_addr& comm_addr)
{
    return true;
}
void process_group_context::collect_cluster_colored_plain_graphs(
                                        const details::colored_plain_graph_list& send_graph,
                                        details::global_sorted_colored_plain_graphs& received_graphs)
{
}
}
