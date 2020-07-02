#pragma once
#include "common/comm/l0/topology/topology_construction_utils.hpp"

namespace ccl
{
    struct context_comm_addr;
}

namespace native
{

class thread_group_ring_topology
{
    thread_group_context& context;
    device_storage& devices_factory;

public:
    static constexpr ccl::device_group_split_type group_id()
    {
        return ccl::device_group_split_type::process;
    }

    static constexpr const char* name()
    {
        return "thread_group_ring_creator";
    }

    thread_group_ring_topology(thread_group_context &ctx,
                               device_storage& devs);

    static size_t default_property_p2p_rating_calculator(const ccl_device &lhs, const ccl_device &rhs);
    static details::adjacency_matrix build_p2p_capability_matrix(std::ostream& out,
                                                                 const ccl::process_aggregated_device_mask_t& per_thread_device_masks,
                                                                 details::p2p_rating_function ping =
                                                                        default_property_p2p_rating_calculator);
    static details::adjacency_matrix build_p2p_capability_matrix(std::ostream& out,
                                                                 const ccl::process_device_indices_t& per_thread_device_indices,
                                                                 details::p2p_rating_function ping =
                                                                        default_property_p2p_rating_calculator);

    bool build(std::ostream& out,
               const ccl::context_comm_addr& context_addr,
               const ccl::process_aggregated_device_mask_t& per_thread_device_masks,
               const details::adjacency_matrix& matrix,
               details::p2p_rating_function ping = default_property_p2p_rating_calculator);
    bool build(std::ostream& out,
               const ccl::context_comm_addr& context_addr,
               const ccl::process_device_indices_t& per_thread_device_indices,
               const details::adjacency_matrix& matrix,
               details::p2p_rating_function ping = default_property_p2p_rating_calculator);
private:
    bool build_specific(std::ostream& out,
                        const ccl::context_comm_addr& context_addr,
                        const ccl::process_device_indices_t& per_thread_device_indices,
                        const details::plain_graph& graph);
    bool build_scale_up_specific(std::ostream& out,
                        const ccl::context_comm_addr& context_addr,
                        const ccl::process_device_indices_t& per_thread_device_indicess,
                        const details::plain_graph_list& graph_list);
};
}
