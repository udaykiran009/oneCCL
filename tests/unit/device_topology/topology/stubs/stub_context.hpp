#pragma once
#include <vector>
#include <string>

#define private public
#define protected public

#include "oneapi/ccl/config.h"
#include "common/comm/l0/topology/ring_topology.hpp"
#include "common/comm/l0/device_community.hpp"
#include "common/comm/l0/context/device_group_ctx.hpp"
#include "common/comm/l0/context/thread_group_ctx.hpp"
#include "common/comm/l0/context/process_group_ctx.hpp"

#undef protected
#undef private

namespace stub {
struct process_context : public native::process_group_context {
    process_context(std::shared_ptr<ccl::host_communicator> communicator)
            : native::process_group_context(communicator) {}

    // stubs override
    void collect_cluster_colored_plain_graphs(
        const native::detail::colored_plain_graph_list& send_graph,
        native::detail::global_sorted_colored_plain_graphs& received_graphs) override {
        if (!stub_received_graphs.empty()) {
            received_graphs = stub_received_graphs;
        }
        /*
        native::process_group_context::collect_cluster_colored_plain_graphs(send_graph,
                                                                            received_graphs);
*/
    }

    // impls
    void set_collect_cluster_colored_plain_graphs(
        native::detail::global_sorted_colored_plain_graphs received_graphs) {
        stub_received_graphs = received_graphs;
    }

    native::detail::global_sorted_colored_plain_graphs stub_received_graphs;
};
} // namespace stub
