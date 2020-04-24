#pragma once
#include "common/comm/l0/topology/topology_construction_utils.hpp"

namespace ccl
{
    struct context_comm_addr;
}

namespace native
{

class device_group_ring_topology
{
    device_group_context&                                               context;
    device_storage&                                                     devices_factory;

public:
    device_group_ring_topology(device_group_context& comm,
                               device_storage& devs);

    static constexpr const char* name()
    {
        return "device_group_ring_creator";
    }

    static size_t default_property_p2p_rating_calculator(const ccl_device &lhs, const ccl_device &rhs);
    static details::adjacency_matrix build_p2p_capability_matrix(std::ostream& out,
                                                                 const ccl::device_indices_t& group_device_indices,
                                                                 details::p2p_rating_function ping =
                                                                        default_property_p2p_rating_calculator);

    static details::adjacency_matrix build_p2p_capability_matrix(std::ostream& out,
                                                                 const ccl::device_mask_t& group_device_masks,
                                                                 details::p2p_rating_function ping =
                                                                        default_property_p2p_rating_calculator);
    bool build(std::ostream& out,
               const ccl::context_comm_addr& comm_addr,
               const ccl::device_mask_t &group_device_masks,
               const details::adjacency_matrix& matrix);
    bool build(std::ostream& out,
               const ccl::context_comm_addr& comm_addr,
               const ccl::device_indices_t& group_device_indices,
               const details::adjacency_matrix& matrix);
private:
    bool build_specific(std::ostream& out,
                        const ccl::context_comm_addr& comm_addr,
                        const ccl::device_indices_t& group_device_indices,
                        const details::plain_graph& graph);
    bool build_specific(std::ostream& out,
                        const ccl::context_comm_addr& comm_addr,
                        const ccl::device_indices_t& group_device_indices,
                        const details::plain_graph_list& graph_list);
};
}
