#pragma once
#include "../topology_utils.hpp"

namespace topology_suite
{

TEST_F(router_fixture, thread_group_topology_test)
{
    using namespace utils;
    using namespace native;

    size_t process_index = 0;
    std::vector<size_t> thread_ids {0, 1};

    constexpr ccl::device_topology_type topology = ccl::device_topology_type::thread_group_ring;
    init_routing_data(process_index, 1, thread_ids.size());
    {
        //emulate last thread barrier creation
        //prepare context
        tg_comm->per_thread_indices = ccl::process_device_indices_t
                                      {
                                          {0,
                                              {
                                                  ccl::device_index_type(0,0, ccl::unused_index_value),
                                                  ccl::device_index_type(0,0, ccl::unused_index_value),
                                                  ccl::device_index_type(0,0, ccl::unused_index_value)
                                              }
                                          },
                                          {1,
                                              {
                                                  ccl::device_index_type(0,1, ccl::unused_index_value),
                                                  ccl::device_index_type(0,1, ccl::unused_index_value),
                                                  ccl::device_index_type(0,2, ccl::unused_index_value)
                                              }
                                          }
                                      };
        for(size_t thread_id : thread_ids)
        {
            create_devices_by_affinity(thread_id, tg_comm->per_thread_indices.find(thread_id)->second);
        }
       // fill_thread_ctx(ctx);


        thread_group_ring_topology top (*tg_comm, *pg_comm->gpu_device_storage);
        std::stringstream ss;
        native::details::adjacency_matrix matrix =
                thread_group_ring_topology::build_p2p_capability_matrix(ss, tg_comm->per_thread_indices);
        output << ss.str() <<"\nResult matrix:\n" << matrix << std::endl;
        ss.clear();

        bool res;
        std::string descr;

        //test device id ring creation
        std::vector<native::details::plain_graph> expected_ring_ids;
        //thread 0
        expected_ring_ids.push_back({
                                        ccl::device_index_type(0,0, ccl::unused_index_value),
                                        ccl::device_index_type(0,0, ccl::unused_index_value),
                                        ccl::device_index_type(0,0, ccl::unused_index_value)
                                    });
        //thread 1
        expected_ring_ids.push_back({
                                        ccl::device_index_type(0,1, ccl::unused_index_value),
                                        ccl::device_index_type(0,1, ccl::unused_index_value),
                                        ccl::device_index_type(0,2, ccl::unused_index_value)
                                    });
        for(size_t thread_id : thread_ids)
        {
            std::tie(res, descr) = check_adj_matrix(matrix, get_device_affinity(thread_id), DEVICE_GROUP_WEIGHT, THREAD_GROUP_WEIGHT);
            if (!res)
            {
                output << "Thread: " << thread_id << ", check_adj_matrix log: " << descr << std::endl;
                UT_ASSERT(res == true, descr);
            }

            std::tie(res, descr) = check_id_ring(matrix, get_device_affinity(thread_id), expected_ring_ids[thread_id]);
            if (!res)
            {
                output << "Thread: " << thread_id << ", check_id_ring log: " << descr << std::endl;
                UT_ASSERT(res == true, descr);
            }
        }

        res = top.build(ss, tg_comm->per_thread_indices, matrix);
        if (!res)
        {
            output << ss.str() << std::endl;
            UT_ASSERT(res, "Cannot build topology: " << ss.str());
        }

        //Check topology
        std::vector<expected_indices_tuple> expected_thread_results;
        //thread 0
        expected_thread_results.push_back(
                                        expected_indices_tuple(
                                                {
                                                    optional_indices{true, {0}, {}},
                                                    optional_indices{true, {1, 2}, {}},
                                                    optional_indices{true, {3}, {}}
                                                }));
        //thread 1
        expected_thread_results.push_back(
                                        expected_indices_tuple(
                                                {
                                                    optional_indices{true, {3, 5}, {}},
                                                    optional_indices{true, {4}, {}},
                                                    optional_indices{true, {0}, {}}
                                                }));
        std::tie(res, descr) =
                        check_multiple_topologies<topology,
                                                  thread_group_context>(tg_comm->thread_device_topology,
                                                                        thread_ids,
                                                                        expected_thread_results);
        UT_ASSERT(res, descr);
    }

    //same device test
    init_routing_data(process_index, 1, thread_ids.size());
    {
        //emulate last thread barrier creation
        //prepare context
        tg_comm->per_thread_indices = ccl::process_device_indices_t
                                      {
                                          {0,
                                              {
                                                  ccl::device_index_type(0,0, ccl::unused_index_value),
                                                  ccl::device_index_type(0,0, ccl::unused_index_value),
                                                  ccl::device_index_type(0,0, ccl::unused_index_value)
                                              }
                                          },
                                          {1,
                                              {
                                                  ccl::device_index_type(0,0, ccl::unused_index_value),
                                                  ccl::device_index_type(0,0, ccl::unused_index_value),
                                                  ccl::device_index_type(0,0, ccl::unused_index_value)
                                              }
                                          }
                                      };
        for(size_t thread_id : thread_ids)
        {
            create_devices_by_affinity(thread_id, tg_comm->per_thread_indices.find(thread_id)->second);
        }


        thread_group_ring_topology top (*tg_comm, *pg_comm->gpu_device_storage);
        std::stringstream ss;
        native::details::adjacency_matrix matrix =
                thread_group_ring_topology::build_p2p_capability_matrix(ss,
                                                                        tg_comm->per_thread_indices);
        output << ss.str() <<"\nResult matrix:\n" << matrix << std::endl;
        ss.clear();

        bool res;
        std::string descr;

        //test device id ring creation
        std::vector<native::details::plain_graph> expected_ring_ids;
        //thread 0
        expected_ring_ids.push_back({
                                        ccl::device_index_type(0,0, ccl::unused_index_value),
                                        ccl::device_index_type(0,0, ccl::unused_index_value),
                                        ccl::device_index_type(0,0, ccl::unused_index_value)
                                    });
        //thread 1
        expected_ring_ids.push_back({
                                        ccl::device_index_type(0,0, ccl::unused_index_value),
                                        ccl::device_index_type(0,0, ccl::unused_index_value),
                                        ccl::device_index_type(0,0, ccl::unused_index_value)
                                    });

        for(size_t thread_id : thread_ids)
        {
            std::tie(res, descr) = check_adj_matrix(matrix, get_device_affinity(thread_id),
                                                    DEVICE_GROUP_WEIGHT, THREAD_GROUP_WEIGHT);
            UT_ASSERT(res == true, descr);
            output << "Thread: " << thread_id << ", check_adj_matrix log: " << descr << std::endl;

            std::tie(res, descr) = check_id_ring(matrix, get_device_affinity(thread_id),
                                                 expected_ring_ids[thread_id]);
            UT_ASSERT(res == true, descr);
            output << "Thread: " << thread_id << ", check_id_ring log: " << descr << std::endl;
        }

        //test topology creation
        res = top.build(ss, tg_comm->per_thread_indices, matrix);
        UT_ASSERT(res, "Cannot build topology: " << ss.str());
        output << ss.str() << std::endl;

        //Check topology
        std::vector<expected_indices_tuple> expected_thread_results;

        //thread 0
        expected_thread_results.push_back(
                                        expected_indices_tuple(
                                                {
                                                    optional_indices{true, {0}, {}},
                                                    optional_indices{true, {1, 2}, {}}
                                                }));
        //thread 1
        expected_thread_results.push_back(
                                        expected_indices_tuple(
                                                {
                                                    optional_indices{false, {}, {}},
                                                    optional_indices{true, {3, 4, 5}, {}},
                                                    optional_indices{true, {0}, {}}
                                                }));
        std::tie(res, descr) =
                        check_multiple_topologies<topology,
                                                  thread_group_context>(tg_comm->thread_device_topology,
                                                                        thread_ids,
                                                                        expected_thread_results);
        UT_ASSERT(res, descr);
    }
}

TEST_F(router_fixture, thread_group_topology_scaleup_test)
{
    using namespace utils;
    using namespace native;
    using namespace native::details;

    size_t process_index = 0;
    std::vector<size_t> thread_ids {0, 1};

    constexpr ccl::device_topology_type topology = ccl::device_topology_type::thread_group_torn_apart_ring;
    init_routing_data(process_index, 1, thread_ids.size());
    {
        output << "TEST: two inaccessible devices for two threads" << std::endl;
        //emulate last thread barrier creation
        //prepare context
        tg_comm->per_thread_indices = ccl::process_device_indices_t
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
        for(size_t thread_id : thread_ids)
        {
            create_devices_by_affinity(thread_id, tg_comm->per_thread_indices.find(thread_id)->second);
        }
       // fill_thread_ctx(ctx);

        adjacency_matrix expected_matrix
                                        {
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
                                            }
                                        };

        std::stringstream ss;
        thread_group_ring_topology top (*tg_comm, *pg_comm->gpu_device_storage);
        native::details::adjacency_matrix matrix =
                thread_group_ring_topology::build_p2p_capability_matrix(ss,
                                                                        tg_comm->per_thread_indices,
                                                                        std::bind(test_custom_p2p_ping,
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
        res = top.build(ss, tg_comm->per_thread_indices, matrix,
                        std::bind(test_custom_p2p_ping,
                                  std::placeholders::_1,
                                  std::placeholders::_2,
                                  expected_matrix));
        UT_ASSERT(res, "Cannot build topology: " << ss.str());
        output << ss.str() << std::endl;

        //Check topology
        std::vector<expected_indices_tuple> expected_thread_results;
        //thread 0
        expected_thread_results.push_back(
                                        expected_indices_tuple(
                                                {
                                                    optional_indices{false,{}, {}},
                                                    optional_indices{false,{},{}},
                                                    optional_indices{false,{},{}},
                                                    optional_indices{false,{},{}},
                                                    optional_indices{false,{},{}},
                                                    optional_indices{false,{},{}},
                                                    optional_indices{false,{},{}},
                                                    optional_indices{true,{0},{0}},
                                                    optional_indices{false,{},{}}
                                                }));
        //thread 1
        expected_thread_results.push_back(
                                        expected_indices_tuple(
                                                {
                                                    optional_indices{false,{}, {}},
                                                    optional_indices{false,{},{}},
                                                    optional_indices{false,{},{}},
                                                    optional_indices{false,{},{}},
                                                    optional_indices{false,{},{}},
                                                    optional_indices{false,{},{}},
                                                    optional_indices{false,{},{}},
                                                    optional_indices{true,{1},{0}},
                                                    optional_indices{false,{},{}}
                                                }));
        std::tie(res, descr) =
                        check_multiple_topologies<topology,
                                                  thread_group_context>(tg_comm->thread_device_topology,
                                                                        thread_ids, expected_thread_results);
        UT_ASSERT(res, descr);
    }


    init_routing_data(process_index, 1, thread_ids.size());
    {
        output << "TEST: two inaccessible groups for two threads" << std::endl;
        //emulate last thread barrier creation
        //prepare context
        tg_comm->per_thread_indices = ccl::process_device_indices_t
                                      {
                                          {0,
                                              {
                                                  ccl::device_index_type(0,0, ccl::unused_index_value),
                                                  ccl::device_index_type(0,1, ccl::unused_index_value)
                                              }
                                          },
                                          {1,
                                              {
                                                  ccl::device_index_type(0,2, ccl::unused_index_value),
                                                  ccl::device_index_type(0,3, ccl::unused_index_value)
                                              }
                                          }
                                      };
        for(size_t thread_id : thread_ids)
        {
            create_devices_by_affinity(thread_id, tg_comm->per_thread_indices.find(thread_id)->second);
        }
       // fill_thread_ctx(ctx);

        adjacency_matrix expected_matrix
                                        {
                                            {
                                                ccl::device_index_type(0,0, ccl::unused_index_value),
                                                {
                                                    {ccl::device_index_type(0,0, ccl::unused_index_value), 1},
                                                    {ccl::device_index_type(0,1, ccl::unused_index_value), 1},
                                                    {ccl::device_index_type(0,2, ccl::unused_index_value), 0},
                                                    {ccl::device_index_type(0,3, ccl::unused_index_value), 0}
                                                }
                                            },
                                            {
                                                ccl::device_index_type(0,1, ccl::unused_index_value),
                                                {
                                                    {ccl::device_index_type(0,0, ccl::unused_index_value), 1},
                                                    {ccl::device_index_type(0,1, ccl::unused_index_value), 1},
                                                    {ccl::device_index_type(0,2, ccl::unused_index_value), 0},
                                                    {ccl::device_index_type(0,3, ccl::unused_index_value), 0}
                                                }
                                            },
                                            {
                                                ccl::device_index_type(0,2, ccl::unused_index_value),
                                                {
                                                    {ccl::device_index_type(0,0, ccl::unused_index_value), 0},
                                                    {ccl::device_index_type(0,1, ccl::unused_index_value), 0},
                                                    {ccl::device_index_type(0,2, ccl::unused_index_value), 1},
                                                    {ccl::device_index_type(0,3, ccl::unused_index_value), 1}
                                                }
                                            },
                                            {
                                                ccl::device_index_type(0,3, ccl::unused_index_value),
                                                {
                                                    {ccl::device_index_type(0,0, ccl::unused_index_value), 0},
                                                    {ccl::device_index_type(0,1, ccl::unused_index_value), 0},
                                                    {ccl::device_index_type(0,2, ccl::unused_index_value), 1},
                                                    {ccl::device_index_type(0,3, ccl::unused_index_value), 1}
                                                }
                                            },
                                        };

        std::stringstream ss;
        thread_group_ring_topology top (*tg_comm, *pg_comm->gpu_device_storage);
        native::details::adjacency_matrix matrix =
                thread_group_ring_topology::build_p2p_capability_matrix(ss,
                                                                        tg_comm->per_thread_indices,
                                                                        std::bind(test_custom_p2p_ping,
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
        res = top.build(ss, tg_comm->per_thread_indices, matrix,
                        std::bind(test_custom_p2p_ping,
                                  std::placeholders::_1,
                                  std::placeholders::_2,
                                  expected_matrix));
        UT_ASSERT(res, "Cannot build topology: " << ss.str());
        output << ss.str() << std::endl;

        //Check topology
        std::vector<expected_indices_tuple> expected_thread_results;
        //thread 0
        expected_thread_results.push_back(
                                        expected_indices_tuple(
                                                {
                                                    optional_indices{true,{0}, {0}},
                                                    optional_indices{false,{},{}},
                                                    optional_indices{false,{},{}},
                                                    optional_indices{false,{},{}},
                                                    optional_indices{false,{},{}},
                                                    optional_indices{false,{},{}},
                                                    optional_indices{false,{},{}},
                                                    optional_indices{true,{1},{1}},
                                                    optional_indices{false,{},{}}
                                                }));
        //thread 1
        expected_thread_results.push_back(
                                        expected_indices_tuple(
                                                {
                                                    optional_indices{true,{2}, {0}},
                                                    optional_indices{false,{},{}},
                                                    optional_indices{false,{},{}},
                                                    optional_indices{false,{},{}},
                                                    optional_indices{false,{},{}},
                                                    optional_indices{false,{},{}},
                                                    optional_indices{false,{},{}},
                                                    optional_indices{true,{3},{1}},
                                                    optional_indices{false,{},{}}
                                                }));
        std::tie(res, descr) =
                        check_multiple_topologies<topology,
                                                  thread_group_context>(tg_comm->thread_device_topology,
                                                                        thread_ids, expected_thread_results);
        UT_ASSERT(res, descr);
    }

    init_routing_data(process_index, 1, thread_ids.size());
    {
        output << "TEST: local group inaccessible devices to merge for two threads" << std::endl;
        //emulate last thread barrier creation
        //prepare context
        tg_comm->per_thread_indices = ccl::process_device_indices_t
                                      {
                                          {0,
                                              {
                                                  ccl::device_index_type(0,0, ccl::unused_index_value),
                                                  ccl::device_index_type(0,1, ccl::unused_index_value),
                                                  ccl::device_index_type(0,2, ccl::unused_index_value),
                                              }
                                          },
                                          {1,
                                              {
                                                  ccl::device_index_type(0,0, ccl::unused_index_value),
                                                  ccl::device_index_type(0,1, ccl::unused_index_value),
                                                  ccl::device_index_type(0,2, ccl::unused_index_value),
                                              }
                                          }
                                      };
        for(size_t thread_id : thread_ids)
        {
            create_devices_by_affinity(thread_id, tg_comm->per_thread_indices.find(thread_id)->second);
        }

        adjacency_matrix expected_matrix
                                        {
                                            {
                                                ccl::device_index_type(0,0, ccl::unused_index_value),
                                                {
                                                    {ccl::device_index_type(0,0, ccl::unused_index_value), 1},
                                                    {ccl::device_index_type(0,1, ccl::unused_index_value), 0},
                                                    {ccl::device_index_type(0,2, ccl::unused_index_value), 1}
                                                }
                                            },
                                            {
                                                ccl::device_index_type(0,1, ccl::unused_index_value),
                                                {
                                                    {ccl::device_index_type(0,0, ccl::unused_index_value), 0},
                                                    {ccl::device_index_type(0,1, ccl::unused_index_value), 1},
                                                    {ccl::device_index_type(0,2, ccl::unused_index_value), 0}
                                                }
                                            },
                                            {
                                                ccl::device_index_type(0,2, ccl::unused_index_value),
                                                {
                                                    {ccl::device_index_type(0,0, ccl::unused_index_value), 1},
                                                    {ccl::device_index_type(0,1, ccl::unused_index_value), 0},
                                                    {ccl::device_index_type(0,2, ccl::unused_index_value), 1}
                                                }
                                            },
                                        };

        std::stringstream ss;
        thread_group_ring_topology top (*tg_comm, *pg_comm->gpu_device_storage);
        native::details::adjacency_matrix matrix =
                thread_group_ring_topology::build_p2p_capability_matrix(ss,
                                                                        tg_comm->per_thread_indices,
                                                                        std::bind(test_custom_p2p_ping,
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
        res = top.build(ss, tg_comm->per_thread_indices, matrix,
                        std::bind(test_custom_p2p_ping,
                                  std::placeholders::_1,
                                  std::placeholders::_2,
                                  expected_matrix));
        UT_ASSERT(res, "Cannot build topology: " << ss.str());
        output << ss.str() << std::endl;

        //Check topology
        std::vector<expected_indices_tuple> expected_thread_results;
        //thread 0
        expected_thread_results.push_back(
                                        expected_indices_tuple(
                                                {
                                                    optional_indices{true,{0}, {0}},
                                                    optional_indices{false,{},{}},
                                                    optional_indices{false,{},{}},
                                                    optional_indices{true,{2,5},{2,1}},
                                                    optional_indices{false,{},{}},
                                                    optional_indices{false,{},{}},
                                                    optional_indices{false,{},{}},
                                                    optional_indices{true,{1,4},{1,0}},
                                                    optional_indices{false,{},{}}
                                                }));
        //thread 1
        expected_thread_results.push_back(
                                        expected_indices_tuple(
                                                {
                                                    optional_indices{false,{}, {}},
                                                    optional_indices{true,{2,3,5},{2,3,1}},
                                                    optional_indices{true,{0},{0}},
                                                    optional_indices{false,{},{}},
                                                    optional_indices{false,{},{}},
                                                    optional_indices{false,{},{}},
                                                    optional_indices{false,{},{}},
                                                    optional_indices{false,{},{}},
                                                    optional_indices{false,{},{}}
                                                }));
        std::tie(res, descr) =
                        check_multiple_topologies<topology,
                                                  thread_group_context>(tg_comm->thread_device_topology,
                                                                        thread_ids, expected_thread_results);
        UT_ASSERT(res, descr);
    }
}

}
