#include <fstream>
#include <vector>
#include <sstream>
#include <iterator>
#include <set>
#include <unistd.h>
#include <limits.h>
#include <gnu/libc-version.h>

#include "common/comm/l0/context/process_group_ctx.hpp"
#include "thread_group_scheduler.hpp"
#include "common/comm/l0/context/thread_group_ctx.hpp"
#include "common/comm/l0/gpu_comm_attr.hpp"
#include "common/comm/host_communicator/host_communicator.hpp"

namespace native {
struct allied_process_group_scheduler {};
struct device_storage {};

process_group_context::process_group_context(std::shared_ptr<ccl::host_communicator> comm)
        : ccl_communicator(comm),
          thread_group_ctx(new thread_group_context) {}

process_group_context::~process_group_context() {}

bool process_group_context::sync_barrier(const ccl::device_mask_t& thread_device_mask,
                                         ccl::context_comm_addr& comm_addr) {
    return sync_barrier(ccl_device_driver::get_device_indices(thread_device_mask), comm_addr);
}

bool process_group_context::sync_barrier(const ccl::device_indices_t& thread_device_indices,
                                         ccl::context_comm_addr& comm_addr) {
    // sync all threads at first - blocking operation
    return thread_group_ctx->sync_barrier(thread_device_indices, comm_addr, *gpu_device_storage);
}

void process_group_context::collect_cluster_colored_plain_graphs(
    const detail::colored_plain_graph_list& send_graph,
    detail::global_sorted_colored_plain_graphs& received_graphs) {}
} // namespace native
