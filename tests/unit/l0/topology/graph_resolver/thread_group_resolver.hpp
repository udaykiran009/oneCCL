#pragma once
#include "../topology_utils.hpp"

namespace topology_graph_resolver
{

TEST_F(router_fixture, thread_graph_resolver_test)
{
    using namespace utils;
    using namespace native;
    using namespace native::details;

    {
        output << "TEST: two inaccessible devices" << std::endl;
        ccl::process_device_indices_t indices
                                      {
                                          {0,
                                              {
                                                  ccl::device_index_type(0,0, ccl::unused_index_value),
                                              }
                                          },
                                          {1,
                                              {
                                                  ccl::device_index_type(0,1, ccl::unused_index_value),
                                              }
                                          }
                                      };
        stub::make_stub_devices(indices);
        adjacency_matrix expected_matrix {
                                            {
                                                ccl::device_index_type(0,0, ccl::unused_index_value),
                                                {
                                                    {ccl::device_index_type(0,0, ccl::unused_index_value), 1},
                                                    {ccl::device_index_type(0,1, ccl::unused_index_value), 0}
                                                }
                                            },
                                            {
                                                ccl::device_index_type(0,1, ccl::unused_index_value),
                                                {
                                                    {ccl::device_index_type(0,0, ccl::unused_index_value), 0},
                                                    {ccl::device_index_type(0,1, ccl::unused_index_value), 1}
                                                }
                                            },
                                        };

        std::stringstream ss;
        adjacency_matrix matrix =
                        thread_group_ring_topology::build_p2p_capability_matrix
                                (ss, indices, std::bind(test_custom_p2p_ping,
                                                           std::placeholders::_1,
                                                           std::placeholders::_2,
                                                           expected_matrix));
        //check matrix
        if (matrix != expected_matrix)
        {
            output << ss.str() << "\nResult initial_matrix:\n"
                   << matrix
                   << "\nExpected matrix:\n"
                   << expected_matrix << std::endl;
            UT_ASSERT(false, "Matrix should be equal to expected_matrix");
        }
        ss.clear();


        bool res;
        std::string descr;

        plain_graph_list graphs = graph_list_resolver(matrix, indices, std::bind(test_custom_p2p_ping,
                                                                                 std::placeholders::_1,
                                                                                 std::placeholders::_2,
                                                                                 expected_matrix));
        std::tie(res, descr) = check_multiple_graph_ring(graphs,
                                                         {
                                                             {
                                                                 ccl::device_index_type(0,0, ccl::unused_index_value),
                                                             },
                                                             {
                                                                 ccl::device_index_type(0,1, ccl::unused_index_value)
                                                             }
                                                         });
    }

    {
        output << "TEST: for merge two groups" << std::endl;
        ccl::process_device_indices_t indices
                                      {
                                          {0,
                                              {
                                                  ccl::device_index_type(0,0, ccl::unused_index_value),
                                                  ccl::device_index_type(0,1, ccl::unused_index_value),
                                                  ccl::device_index_type(0,2, ccl::unused_index_value),
                                                  ccl::device_index_type(0,3, ccl::unused_index_value),
                                              }
                                          },
                                          {1,
                                              {
                                                  ccl::device_index_type(0,4, ccl::unused_index_value),
                                                  ccl::device_index_type(0,5, ccl::unused_index_value),
                                              }
                                          }
                                      };
        stub::make_stub_devices(indices);
        adjacency_matrix expected_matrix {
                                            {
                                                ccl::device_index_type(0,0, ccl::unused_index_value),
                                                {
                                                    {ccl::device_index_type(0,0, ccl::unused_index_value), 1},
                                                    {ccl::device_index_type(0,1, ccl::unused_index_value), 1},
                                                    {ccl::device_index_type(0,2, ccl::unused_index_value), 1},
                                                    {ccl::device_index_type(0,3, ccl::unused_index_value), 1},
                                                    {ccl::device_index_type(0,4, ccl::unused_index_value), 1},
                                                    {ccl::device_index_type(0,5, ccl::unused_index_value), 1}

                                                }
                                            },
                                            {
                                                ccl::device_index_type(0,1, ccl::unused_index_value),
                                                {
                                                    {ccl::device_index_type(0,0, ccl::unused_index_value), 1},
                                                    {ccl::device_index_type(0,1, ccl::unused_index_value), 1},
                                                    {ccl::device_index_type(0,2, ccl::unused_index_value), 1},
                                                    {ccl::device_index_type(0,3, ccl::unused_index_value), 1},
                                                    {ccl::device_index_type(0,4, ccl::unused_index_value), 1},
                                                    {ccl::device_index_type(0,5, ccl::unused_index_value), 1}

                                                }
                                            },
                                            {
                                                ccl::device_index_type(0,2, ccl::unused_index_value),
                                                {
                                                    {ccl::device_index_type(0,0, ccl::unused_index_value), 1},
                                                    {ccl::device_index_type(0,1, ccl::unused_index_value), 1},
                                                    {ccl::device_index_type(0,2, ccl::unused_index_value), 1},
                                                    {ccl::device_index_type(0,3, ccl::unused_index_value), 1},
                                                    {ccl::device_index_type(0,4, ccl::unused_index_value), 1},
                                                    {ccl::device_index_type(0,5, ccl::unused_index_value), 1}

                                                }
                                            },
                                            {
                                                ccl::device_index_type(0,3, ccl::unused_index_value),
                                                {
                                                    {ccl::device_index_type(0,0, ccl::unused_index_value), 1},
                                                    {ccl::device_index_type(0,1, ccl::unused_index_value), 1},
                                                    {ccl::device_index_type(0,2, ccl::unused_index_value), 1},
                                                    {ccl::device_index_type(0,3, ccl::unused_index_value), 1},
                                                    {ccl::device_index_type(0,4, ccl::unused_index_value), 1},
                                                    {ccl::device_index_type(0,5, ccl::unused_index_value), 1}

                                                }
                                            },
                                            {
                                                ccl::device_index_type(0,4, ccl::unused_index_value),
                                                {
                                                    {ccl::device_index_type(0,0, ccl::unused_index_value), 1},
                                                    {ccl::device_index_type(0,1, ccl::unused_index_value), 1},
                                                    {ccl::device_index_type(0,2, ccl::unused_index_value), 1},
                                                    {ccl::device_index_type(0,3, ccl::unused_index_value), 1},
                                                    {ccl::device_index_type(0,4, ccl::unused_index_value), 1},
                                                    {ccl::device_index_type(0,5, ccl::unused_index_value), 1}

                                                }
                                            },
                                            {
                                                ccl::device_index_type(0,5, ccl::unused_index_value),
                                                {
                                                    {ccl::device_index_type(0,0, ccl::unused_index_value), 1},
                                                    {ccl::device_index_type(0,1, ccl::unused_index_value), 1},
                                                    {ccl::device_index_type(0,2, ccl::unused_index_value), 1},
                                                    {ccl::device_index_type(0,3, ccl::unused_index_value), 1},
                                                    {ccl::device_index_type(0,4, ccl::unused_index_value), 1},
                                                    {ccl::device_index_type(0,5, ccl::unused_index_value), 1}

                                                }
                                            },
                                        };

        std::stringstream ss;
        adjacency_matrix matrix =
                        thread_group_ring_topology::build_p2p_capability_matrix
                                (ss, indices, std::bind(test_custom_p2p_ping,
                                                           std::placeholders::_1,
                                                           std::placeholders::_2,
                                                           expected_matrix));
        //check matrix
        if (matrix != expected_matrix)
        {
            output << ss.str() << "\nResult initial_matrix:\n"
                   << matrix
                   << "\nExpected matrix:\n"
                   << expected_matrix << std::endl;
            UT_ASSERT(false, "Matrix should be equal to expected_matrix");
        }
        ss.clear();


        bool res;
        std::string descr;

        plain_graph_list graphs = graph_list_resolver(matrix, indices, std::bind(test_custom_p2p_ping,
                                                                                 std::placeholders::_1,
                                                                                 std::placeholders::_2,
                                                                                 expected_matrix));
        std::tie(res, descr) = check_multiple_graph_ring(graphs,
                                                         {
                                                             {
                                                                 ccl::device_index_type(0,0, ccl::unused_index_value),
                                                                 ccl::device_index_type(0,1, ccl::unused_index_value),
                                                                 ccl::device_index_type(0,2, ccl::unused_index_value),
                                                                 ccl::device_index_type(0,3, ccl::unused_index_value),
                                                                 ccl::device_index_type(0,4, ccl::unused_index_value),
                                                                 ccl::device_index_type(0,5, ccl::unused_index_value)
                                                             }
                                                         });
    }
}
}
