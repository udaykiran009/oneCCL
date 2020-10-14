#pragma once
#include "../topology_utils.hpp"

namespace topology_graph_resolver {

TEST_F(router_fixture, graph_resolver_default_creator_test) {
    using namespace utils;
    using namespace native;
    using namespace native::detail;

    {
        ccl::device_indices_t indices{ ccl::device_index_type(0, 0, ccl::unused_index_value),
                                       ccl::device_index_type(0, 0, ccl::unused_index_value),
                                       ccl::device_index_type(0, 0, ccl::unused_index_value) };
        stub::make_stub_devices(indices);

        std::stringstream ss;
        adjacency_matrix initial_matrix = device_group_ring_topology::build_p2p_capability_matrix(
            ss, indices, all_p2p_accessible);

        adjacency_matrix custom_matrix =
            device_group_ring_topology::build_p2p_capability_matrix(ss,
                                                                    indices,
                                                                    std::bind(test_custom_p2p_ping,
                                                                              std::placeholders::_1,
                                                                              std::placeholders::_2,
                                                                              initial_matrix));

        //check matrix
        UT_ASSERT(custom_matrix == initial_matrix, "Initil matrix should be equal");

        plain_graph_list graph = graph_list_resolver(custom_matrix, indices);
        UT_ASSERT(graph.size() == 1, "plain graph list size should be 1");
    }
}

TEST_F(router_fixture, graph_resolver_creator_test_devices_2_per_groups_2_non_p2p) {
    using namespace utils;
    using namespace native;
    using namespace native::detail;

        ccl::device_indices_t indices{ ccl::device_index_type(0, 0, ccl::unused_index_value),
                                       ccl::device_index_type(0, 1, ccl::unused_index_value),
                                       ccl::device_index_type(0, 2, ccl::unused_index_value),
                                       ccl::device_index_type(0, 3, ccl::unused_index_value) };
        stub::make_stub_devices(indices);

        adjacency_matrix expected_matrix{
            { ccl::device_index_type(0, 0, ccl::unused_index_value),
              { { ccl::device_index_type(0, 0, ccl::unused_index_value), 1 },
                { ccl::device_index_type(0, 1, ccl::unused_index_value), 1 },
                { ccl::device_index_type(0, 2, ccl::unused_index_value), 0 },
                { ccl::device_index_type(0, 3, ccl::unused_index_value), 0 } } },
            { ccl::device_index_type(0, 1, ccl::unused_index_value),
              { { ccl::device_index_type(0, 0, ccl::unused_index_value), 1 },
                { ccl::device_index_type(0, 1, ccl::unused_index_value), 1 },
                { ccl::device_index_type(0, 2, ccl::unused_index_value), 0 },
                { ccl::device_index_type(0, 3, ccl::unused_index_value), 0 } } },
            { ccl::device_index_type(0, 2, ccl::unused_index_value),
              { { ccl::device_index_type(0, 0, ccl::unused_index_value), 0 },
                { ccl::device_index_type(0, 1, ccl::unused_index_value), 0 },
                { ccl::device_index_type(0, 2, ccl::unused_index_value), 1 },
                { ccl::device_index_type(0, 3, ccl::unused_index_value), 1 } } },
            { ccl::device_index_type(0, 3, ccl::unused_index_value),
              { { ccl::device_index_type(0, 0, ccl::unused_index_value), 0 },
                { ccl::device_index_type(0, 1, ccl::unused_index_value), 0 },
                { ccl::device_index_type(0, 2, ccl::unused_index_value), 1 },
                { ccl::device_index_type(0, 3, ccl::unused_index_value), 1 } } }
        };

        std::stringstream ss;
        adjacency_matrix matrix =
            device_group_ring_topology::build_p2p_capability_matrix(ss,
                                                                    indices,
                                                                    std::bind(test_custom_p2p_ping,
                                                                              std::placeholders::_1,
                                                                              std::placeholders::_2,
                                                                              expected_matrix));
        //check matrix
        if (matrix != expected_matrix) {
            output << ss.str() << "\nResult initial_matrix:\n"
                   << matrix << "\nExpected matrix:\n"
                   << expected_matrix << std::endl;
            UT_ASSERT(false, "Matrix should be equal to expected_matrix");
        }
        ss.clear();

        bool res;
        std::string descr;

        plain_graph_list graphs = graph_list_resolver(matrix, indices);
        std::tie(res, descr) = check_multiple_graph_ring(
            graphs,
            { { ccl::device_index_type(0, 0, ccl::unused_index_value),
                ccl::device_index_type(0, 1, ccl::unused_index_value) },
              { ccl::device_index_type(0, 2, ccl::unused_index_value),
                ccl::device_index_type(0, 3, ccl::unused_index_value) } });

}

TEST_F(router_fixture, graph_resolver_creator_test_devices_2_per_groups_1_non_p2p) {
    using namespace utils;
    using namespace native;
    using namespace native::detail;
        ccl::device_indices_t indices{ ccl::device_index_type(0, 0, ccl::unused_index_value),
                                       ccl::device_index_type(0, 1, ccl::unused_index_value) };
        stub::make_stub_devices(indices);

        adjacency_matrix expected_matrix{
            { ccl::device_index_type(0, 0, ccl::unused_index_value),
              { { ccl::device_index_type(0, 0, ccl::unused_index_value), 1 },
                { ccl::device_index_type(0, 1, ccl::unused_index_value), 0 } } },
            { ccl::device_index_type(0, 1, ccl::unused_index_value),
              { { ccl::device_index_type(0, 0, ccl::unused_index_value), 0 },
                { ccl::device_index_type(0, 1, ccl::unused_index_value), 1 } } }
        };

        std::stringstream ss;
        adjacency_matrix matrix =
            device_group_ring_topology::build_p2p_capability_matrix(ss,
                                                                    indices,
                                                                    std::bind(test_custom_p2p_ping,
                                                                              std::placeholders::_1,
                                                                              std::placeholders::_2,
                                                                              expected_matrix));
        //check matrix
        if (matrix != expected_matrix) {
            output << ss.str() << "\nResult initial_matrix:\n"
                   << matrix << "\nExpected matrix:\n"
                   << expected_matrix << std::endl;
            UT_ASSERT(false, "Matrix should be equal to expected_matrix");
        }
        ss.clear();

        bool res;
        std::string descr;

        plain_graph_list graphs = graph_list_resolver(matrix, indices);
        std::tie(res, descr) = check_multiple_graph_ring(
            graphs,
            { { ccl::device_index_type(0, 0, ccl::unused_index_value) },
              { ccl::device_index_type(0, 1, ccl::unused_index_value) } });
}

TEST_F(router_fixture, graph_resolver_creator_test_devices_10_per_groups_multi_assymmetric_p2p) {
    using namespace utils;
    using namespace native;
    using namespace native::detail;
        ccl::device_indices_t indices{ ccl::device_index_type(0, 0, ccl::unused_index_value),
                                       ccl::device_index_type(0, 1, ccl::unused_index_value),
                                       ccl::device_index_type(0, 2, ccl::unused_index_value),
                                       ccl::device_index_type(0, 3, ccl::unused_index_value),
                                       ccl::device_index_type(0, 4, ccl::unused_index_value),
                                       ccl::device_index_type(0, 5, ccl::unused_index_value),
                                       ccl::device_index_type(0, 6, ccl::unused_index_value),
                                       ccl::device_index_type(0, 7, ccl::unused_index_value),
                                       ccl::device_index_type(0, 8, ccl::unused_index_value),
                                       ccl::device_index_type(0, 9, ccl::unused_index_value) };
        stub::make_stub_devices(indices);
        adjacency_matrix expected_matrix{
            { ccl::device_index_type(0, 0, ccl::unused_index_value),
              { { ccl::device_index_type(0, 0, ccl::unused_index_value), 1 },
                { ccl::device_index_type(0, 1, ccl::unused_index_value), 1 },
                { ccl::device_index_type(0, 2, ccl::unused_index_value), 0 },
                { ccl::device_index_type(0, 3, ccl::unused_index_value), 0 },
                { ccl::device_index_type(0, 4, ccl::unused_index_value), 0 },
                { ccl::device_index_type(0, 5, ccl::unused_index_value), 0 },
                { ccl::device_index_type(0, 6, ccl::unused_index_value), 0 },
                { ccl::device_index_type(0, 7, ccl::unused_index_value), 0 },
                { ccl::device_index_type(0, 8, ccl::unused_index_value), 0 },
                { ccl::device_index_type(0, 9, ccl::unused_index_value), 1 } } },
            { ccl::device_index_type(0, 1, ccl::unused_index_value),
              { { ccl::device_index_type(0, 0, ccl::unused_index_value), 1 },
                { ccl::device_index_type(0, 1, ccl::unused_index_value), 1 },
                { ccl::device_index_type(0, 2, ccl::unused_index_value), 0 },
                { ccl::device_index_type(0, 3, ccl::unused_index_value), 0 },
                { ccl::device_index_type(0, 4, ccl::unused_index_value), 0 },
                { ccl::device_index_type(0, 5, ccl::unused_index_value), 0 },
                { ccl::device_index_type(0, 6, ccl::unused_index_value), 0 },
                { ccl::device_index_type(0, 7, ccl::unused_index_value), 0 },
                { ccl::device_index_type(0, 8, ccl::unused_index_value), 0 },
                { ccl::device_index_type(0, 9, ccl::unused_index_value), 1 } } },
            { ccl::device_index_type(0, 2, ccl::unused_index_value),
              { { ccl::device_index_type(0, 0, ccl::unused_index_value), 0 },
                { ccl::device_index_type(0, 1, ccl::unused_index_value), 0 },
                { ccl::device_index_type(0, 2, ccl::unused_index_value), 1 },
                { ccl::device_index_type(0, 3, ccl::unused_index_value), 0 },
                { ccl::device_index_type(0, 4, ccl::unused_index_value), 0 },
                { ccl::device_index_type(0, 5, ccl::unused_index_value), 0 },
                { ccl::device_index_type(0, 6, ccl::unused_index_value), 0 },
                { ccl::device_index_type(0, 7, ccl::unused_index_value), 0 },
                { ccl::device_index_type(0, 8, ccl::unused_index_value), 0 },
                { ccl::device_index_type(0, 9, ccl::unused_index_value), 0 } } },
            { ccl::device_index_type(0, 3, ccl::unused_index_value),
              { { ccl::device_index_type(0, 0, ccl::unused_index_value), 0 },
                { ccl::device_index_type(0, 1, ccl::unused_index_value), 0 },
                { ccl::device_index_type(0, 2, ccl::unused_index_value), 0 },
                { ccl::device_index_type(0, 3, ccl::unused_index_value), 1 },
                { ccl::device_index_type(0, 4, ccl::unused_index_value), 1 },
                { ccl::device_index_type(0, 5, ccl::unused_index_value), 1 },
                { ccl::device_index_type(0, 6, ccl::unused_index_value), 1 },
                { ccl::device_index_type(0, 7, ccl::unused_index_value), 0 },
                { ccl::device_index_type(0, 8, ccl::unused_index_value), 0 },
                { ccl::device_index_type(0, 9, ccl::unused_index_value), 0 } } },
            { ccl::device_index_type(0, 4, ccl::unused_index_value),
              { { ccl::device_index_type(0, 0, ccl::unused_index_value), 0 },
                { ccl::device_index_type(0, 1, ccl::unused_index_value), 0 },
                { ccl::device_index_type(0, 2, ccl::unused_index_value), 0 },
                { ccl::device_index_type(0, 3, ccl::unused_index_value), 1 },
                { ccl::device_index_type(0, 4, ccl::unused_index_value), 1 },
                { ccl::device_index_type(0, 5, ccl::unused_index_value), 1 },
                { ccl::device_index_type(0, 6, ccl::unused_index_value), 1 },
                { ccl::device_index_type(0, 7, ccl::unused_index_value), 0 },
                { ccl::device_index_type(0, 8, ccl::unused_index_value), 0 },
                { ccl::device_index_type(0, 9, ccl::unused_index_value), 0 } } },
            { ccl::device_index_type(0, 5, ccl::unused_index_value),
              { { ccl::device_index_type(0, 0, ccl::unused_index_value), 0 },
                { ccl::device_index_type(0, 1, ccl::unused_index_value), 0 },
                { ccl::device_index_type(0, 2, ccl::unused_index_value), 0 },
                { ccl::device_index_type(0, 3, ccl::unused_index_value), 1 },
                { ccl::device_index_type(0, 4, ccl::unused_index_value), 1 },
                { ccl::device_index_type(0, 5, ccl::unused_index_value), 1 },
                { ccl::device_index_type(0, 6, ccl::unused_index_value), 1 },
                { ccl::device_index_type(0, 7, ccl::unused_index_value), 0 },
                { ccl::device_index_type(0, 8, ccl::unused_index_value), 0 },
                { ccl::device_index_type(0, 9, ccl::unused_index_value), 0 } } },
            { ccl::device_index_type(0, 6, ccl::unused_index_value),
              { { ccl::device_index_type(0, 0, ccl::unused_index_value), 0 },
                { ccl::device_index_type(0, 1, ccl::unused_index_value), 0 },
                { ccl::device_index_type(0, 2, ccl::unused_index_value), 0 },
                { ccl::device_index_type(0, 3, ccl::unused_index_value), 1 },
                { ccl::device_index_type(0, 4, ccl::unused_index_value), 1 },
                { ccl::device_index_type(0, 5, ccl::unused_index_value), 1 },
                { ccl::device_index_type(0, 6, ccl::unused_index_value), 1 },
                { ccl::device_index_type(0, 7, ccl::unused_index_value), 0 },
                { ccl::device_index_type(0, 8, ccl::unused_index_value), 0 },
                { ccl::device_index_type(0, 9, ccl::unused_index_value), 0 } } },
            { ccl::device_index_type(0, 7, ccl::unused_index_value),
              { { ccl::device_index_type(0, 0, ccl::unused_index_value), 0 },
                { ccl::device_index_type(0, 1, ccl::unused_index_value), 0 },
                { ccl::device_index_type(0, 2, ccl::unused_index_value), 0 },
                { ccl::device_index_type(0, 3, ccl::unused_index_value), 0 },
                { ccl::device_index_type(0, 4, ccl::unused_index_value), 0 },
                { ccl::device_index_type(0, 5, ccl::unused_index_value), 0 },
                { ccl::device_index_type(0, 6, ccl::unused_index_value), 0 },
                { ccl::device_index_type(0, 7, ccl::unused_index_value), 1 },
                { ccl::device_index_type(0, 8, ccl::unused_index_value), 0 },
                { ccl::device_index_type(0, 9, ccl::unused_index_value), 0 } } },
            { ccl::device_index_type(0, 8, ccl::unused_index_value),
              { { ccl::device_index_type(0, 0, ccl::unused_index_value), 0 },
                { ccl::device_index_type(0, 1, ccl::unused_index_value), 0 },
                { ccl::device_index_type(0, 2, ccl::unused_index_value), 0 },
                { ccl::device_index_type(0, 3, ccl::unused_index_value), 0 },
                { ccl::device_index_type(0, 4, ccl::unused_index_value), 0 },
                { ccl::device_index_type(0, 5, ccl::unused_index_value), 0 },
                { ccl::device_index_type(0, 6, ccl::unused_index_value), 0 },
                { ccl::device_index_type(0, 7, ccl::unused_index_value), 0 },
                { ccl::device_index_type(0, 8, ccl::unused_index_value), 1 },
                { ccl::device_index_type(0, 9, ccl::unused_index_value), 0 } } },
            { ccl::device_index_type(0, 9, ccl::unused_index_value),
              { { ccl::device_index_type(0, 0, ccl::unused_index_value), 1 },
                { ccl::device_index_type(0, 1, ccl::unused_index_value), 1 },
                { ccl::device_index_type(0, 2, ccl::unused_index_value), 0 },
                { ccl::device_index_type(0, 3, ccl::unused_index_value), 0 },
                { ccl::device_index_type(0, 4, ccl::unused_index_value), 0 },
                { ccl::device_index_type(0, 5, ccl::unused_index_value), 0 },
                { ccl::device_index_type(0, 6, ccl::unused_index_value), 0 },
                { ccl::device_index_type(0, 7, ccl::unused_index_value), 0 },
                { ccl::device_index_type(0, 8, ccl::unused_index_value), 0 },
                { ccl::device_index_type(0, 9, ccl::unused_index_value), 1 } } },
        };

        std::stringstream ss;
        adjacency_matrix matrix =
            device_group_ring_topology::build_p2p_capability_matrix(ss,
                                                                    indices,
                                                                    std::bind(test_custom_p2p_ping,
                                                                              std::placeholders::_1,
                                                                              std::placeholders::_2,
                                                                              expected_matrix));
        //check matrix
        if (matrix != expected_matrix) {
            output << ss.str() << "\nResult initial_matrix:\n"
                   << matrix << "\nExpected matrix:\n"
                   << expected_matrix << std::endl;
            UT_ASSERT(false, "Matrix should be equal to expected_matrix");
        }
        ss.clear();

        bool res;
        std::string descr;

        plain_graph_list graphs = graph_list_resolver(matrix, indices);
        std::tie(res, descr) = check_multiple_graph_ring(
            graphs,
            { { ccl::device_index_type(0, 0, ccl::unused_index_value),
                ccl::device_index_type(0, 1, ccl::unused_index_value),
                ccl::device_index_type(0, 9, ccl::unused_index_value) },
              { ccl::device_index_type(0, 2, ccl::unused_index_value) },
              { ccl::device_index_type(0, 3, ccl::unused_index_value),
                ccl::device_index_type(0, 4, ccl::unused_index_value),
                ccl::device_index_type(0, 5, ccl::unused_index_value),
                ccl::device_index_type(0, 6, ccl::unused_index_value) },
              { ccl::device_index_type(0, 7, ccl::unused_index_value) },
              { ccl::device_index_type(0, 8, ccl::unused_index_value) } });
}

TEST_F(router_fixture, graph_resolver_creator_test_subdevices_2_per_groups_2_non_p2p) {
    using namespace utils;
    using namespace native;
    using namespace native::detail;
        ccl::device_indices_t indices{ ccl::device_index_type(0, 0, 0),
                                       ccl::device_index_type(0, 0, 1),
                                       ccl::device_index_type(0, 1, 0),
                                       ccl::device_index_type(0, 1, 1) };
        ccl::device_indices_t full_stub_indices = indices;
        full_stub_indices.insert({ ccl::device_index_type(0, 0, 0),
                                   ccl::device_index_type(0, 0, 1),
                                   ccl::device_index_type(0, 1, 0),
                                   ccl::device_index_type(0, 1, 1) });
        stub::make_stub_devices(full_stub_indices);
        /* adjacency_matrix expected_matrix {
                                            {
                                                ccl::device_index_type(0,0, ccl::unused_index_value),
                                                {
                                                    {ccl::device_index_type(0,0,ccl::unused_index_value), 1},
                                                    {ccl::device_index_type(0,0,0), 1},
                                                    {ccl::device_index_type(0,0,1), 1},
                                                    {ccl::device_index_type(0,1,ccl::unused_index_value), 0},
                                                    {ccl::device_index_type(0,1,0), 0},
                                                    {ccl::device_index_type(0,1,1), 0}
                                                }
                                            },
                                            {
                                                ccl::device_index_type(0,0,0),
                                                {
                                                    {ccl::device_index_type(0,0,ccl::unused_index_value), 1},
                                                    {ccl::device_index_type(0,0,0), 1},
                                                    {ccl::device_index_type(0,0,1), 1},
                                                    {ccl::device_index_type(0,1,ccl::unused_index_value), 0},
                                                    {ccl::device_index_type(0,1,0), 0},
                                                    {ccl::device_index_type(0,1,1), 0}
                                                }
                                            },
                                            {
                                                ccl::device_index_type(0,0,1),
                                                {
                                                    {ccl::device_index_type(0,0,ccl::unused_index_value), 1},
                                                    {ccl::device_index_type(0,0,0), 1},
                                                    {ccl::device_index_type(0,0,1), 1},
                                                    {ccl::device_index_type(0,1,ccl::unused_index_value), 0},
                                                    {ccl::device_index_type(0,1,0), 0},
                                                    {ccl::device_index_type(0,1,1), 0}
                                                }
                                            },
                                            {
                                                ccl::device_index_type(0,1, ccl::unused_index_value),
                                                {
                                                    {ccl::device_index_type(0,0,ccl::unused_index_value), 0},
                                                    {ccl::device_index_type(0,0,0), 0},
                                                    {ccl::device_index_type(0,0,1), 0},
                                                    {ccl::device_index_type(0,1,ccl::unused_index_value), 1},
                                                    {ccl::device_index_type(0,1,0), 1},
                                                    {ccl::device_index_type(0,1,1), 1}
                                                }
                                            },
                                            {
                                                ccl::device_index_type(0,1,0),
                                                {
                                                    {ccl::device_index_type(0,0,ccl::unused_index_value), 0},
                                                    {ccl::device_index_type(0,0,0), 0},
                                                    {ccl::device_index_type(0,0,1), 0},
                                                    {ccl::device_index_type(0,1,ccl::unused_index_value), 1},
                                                    {ccl::device_index_type(0,1,0), 1},
                                                    {ccl::device_index_type(0,1,1), 1}
                                                }
                                            },
                                            {
                                                ccl::device_index_type(0,1,1),
                                                {
                                                    {ccl::device_index_type(0,0,ccl::unused_index_value), 0},
                                                    {ccl::device_index_type(0,0,0), 0},
                                                    {ccl::device_index_type(0,0,1), 0},
                                                    {ccl::device_index_type(0,1,ccl::unused_index_value), 1},
                                                    {ccl::device_index_type(0,1,0), 1},
                                                    {ccl::device_index_type(0,1,1), 1}
                                                }
                                            },
                                        };*/
        adjacency_matrix expected_matrix{
            { ccl::device_index_type(0, 0, 0),
              { { ccl::device_index_type(0, 0, 0), 1 },
                { ccl::device_index_type(0, 0, 1), 1 },
                { ccl::device_index_type(0, 1, 0), 0 },
                { ccl::device_index_type(0, 1, 1), 0 } } },
            { ccl::device_index_type(0, 0, 1),
              { { ccl::device_index_type(0, 0, 0), 1 },
                { ccl::device_index_type(0, 0, 1), 1 },
                { ccl::device_index_type(0, 1, 0), 0 },
                { ccl::device_index_type(0, 1, 1), 0 } } },
            { ccl::device_index_type(0, 1, 0),
              { { ccl::device_index_type(0, 0, 0), 0 },
                { ccl::device_index_type(0, 0, 1), 0 },
                { ccl::device_index_type(0, 1, 0), 1 },
                { ccl::device_index_type(0, 1, 1), 1 } } },
            { ccl::device_index_type(0, 1, 1),
              { { ccl::device_index_type(0, 0, 0), 0 },
                { ccl::device_index_type(0, 0, 1), 0 },
                { ccl::device_index_type(0, 1, 0), 1 },
                { ccl::device_index_type(0, 1, 1), 1 } } },
        };
        std::stringstream ss;
        adjacency_matrix matrix;
        try {
            matrix = device_group_ring_topology::build_p2p_capability_matrix(
                ss,
                full_stub_indices,
                std::bind(test_custom_p2p_ping,
                          std::placeholders::_1,
                          std::placeholders::_2,
                          expected_matrix));
        }
        catch (const std::exception& ex) {
            output << ss.str() << "\nCannot build matrix: " << ex.what() << std::endl;
            UT_ASSERT(false, "Cannot build matrix");
        }

        //check matrix
        if (matrix != expected_matrix) {
            output << ss.str() << "\nResult initial_matrix:\n"
                   << matrix << "\nExpected matrix:\n"
                   << expected_matrix << std::endl;
            UT_ASSERT(false, "Matrix should be equal to expected_matrix");
        }
        ss.clear();

        bool res;
        std::string descr;

        plain_graph_list graphs = graph_list_resolver(matrix, indices);
        std::tie(res, descr) = check_multiple_graph_ring(
            graphs,
            { { ccl::device_index_type(0, 0, 0), ccl::device_index_type(0, 0, 1) },
              { ccl::device_index_type(0, 1, 0), ccl::device_index_type(0, 1, 1) } });
}
} // namespace topology_graph_resolver
