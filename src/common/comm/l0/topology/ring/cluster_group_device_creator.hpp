#pragma once
#include "common/comm/l0/topology/topology_construction_utils.hpp"

namespace ccl {
struct context_comm_addr;
}

namespace native {

class cluster_group_device_creator {
    size_t process_index;
    size_t process_size;
    process_group_context& context;
    device_storage& devices_factory;

public:
    static constexpr ccl::group_split_type group_id() {
        return ccl::group_split_type::cluster;
    }

    static constexpr const char* name() {
        return "cluster_group_device_creator";
    }

    cluster_group_device_creator(size_t process_idx,
                                 size_t process_nums,
                                 process_group_context& ctx,
                                 device_storage& devs);

    static size_t default_property_p2p_rating_calculator(const ccl_device& lhs,
                                                         const ccl_device& rhs);

    static details::adjacency_matrix build_p2p_capability_matrix(
        std::ostream& out,
        const ccl::process_device_indices_t& single_node_device_indices,
        details::p2p_rating_function ping = default_property_p2p_rating_calculator);
    bool build_all(std::ostream& out,
                   const ccl::context_comm_addr& comm_addr,
                   const ccl::process_device_indices_t& cur_process_per_thread_device_indices,
                   const details::adjacency_matrix& single_node_matrix,
                   details::p2p_rating_function ping = default_property_p2p_rating_calculator);

    template <ccl::device_topology_type class_id>
    bool build_impl(
        std::ostream& out,
        const ccl::context_comm_addr& comm_addr,
        const ccl::process_device_indices_t& cur_process_per_thread_device_indices,
        const details::adjacency_matrix& single_node_matrix,
        const std::vector<std::vector<details::colored_indexed_data<size_t>>>& syntetic_devices,
        details::colored_plain_graph_list& graph_list,
        std::map<size_t, size_t> process_device_rank_offset,
        size_t cluster_device_total_size,
        details::p2p_rating_function ping = default_property_p2p_rating_calculator);
};
} // namespace native
