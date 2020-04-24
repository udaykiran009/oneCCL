#pragma once
#include "../topology_utils.hpp"

namespace topology_suite
{

TEST_F(router_fixture, device_group_topology_test)
{
    using namespace utils;
    using namespace native;

    size_t process_index = 0;
    size_t thread_index = 0;

    constexpr ccl::device_topology_type topology = ccl::device_topology_type::device_group_ring;
    {
        init_routing_data(process_index, 1, 1);
        auto thread_device_list = create_devices_by_affinity(thread_index,
                                                             {
                                                                 ccl::device_index_type(0,0, ccl::unused_index_value),
                                                                 ccl::device_index_type(0,0, ccl::unused_index_value),
                                                                 ccl::device_index_type(0,0, ccl::unused_index_value)
                                                             });
        device_group_ring_topology top(*tg_comm->get_device_group_ctx(thread_index),
                                       *pg_comm->gpu_device_storage);
        {
            std::stringstream ss;
            native::details::adjacency_matrix matrix =
                    device_group_ring_topology::build_p2p_capability_matrix(ss,
                                                                            get_device_affinity(thread_index));
            output << ss.str() <<"\nResult matrix:\n" << matrix << std::endl;
            ss.clear();

            //check matrix
            bool res;
            std::string descr;
            std::tie(res, descr) = check_adj_matrix(matrix, get_device_affinity(thread_index),
                                                    DEVICE_GROUP_WEIGHT, DEVICE_GROUP_WEIGHT);
            UT_ASSERT(res == true, descr);
            output << descr << std::endl;

            std::tie(res, descr) = check_id_ring(matrix, get_device_affinity(thread_index),
                                                 native::details::plain_graph{
                                                        ccl::device_index_type(0,0, ccl::unused_index_value),
                                                        ccl::device_index_type(0,0, ccl::unused_index_value),
                                                        ccl::device_index_type(0,0, ccl::unused_index_value)
                                                                            });
            UT_ASSERT(res == true, descr);

/* built alredy -- skip
            res = top.build(ss, thread_index, get_device_affinity(thread_index), matrix);
            UT_ASSERT(res, "Cannot build topology: " << ss.str());
*/
            output << ss.str() << std::endl;

            native::details::printer<topology> p;
            ccl_tuple_for_each(*tg_comm->get_device_group_ctx(thread_index)->get_group_topology<topology>()->devices, p);
            output << "Router:\n" << p.to_string();

            //Check topology
            std::tie(res, descr) = check_topology<topology>(tg_comm->get_device_group_ctx(thread_index)->get_group_topology<topology>()->devices,
                                                            expected_indices_tuple({ optional_indices{true, {0}, {} },
                                                                                    optional_indices{true, {1,2}, {}}}));
            UT_ASSERT(res, descr);
        }
    }

    {
        init_routing_data(process_index, 1, 1);
        auto thread_device_list = create_devices_by_affinity(thread_index,
                                                             {
                                                                ccl::device_index_type(0,0, ccl::unused_index_value),
                                                                ccl::device_index_type(0,1, ccl::unused_index_value),
                                                                ccl::device_index_type(0,2, ccl::unused_index_value)
                                                             });
        device_group_ring_topology top(*tg_comm->get_device_group_ctx(thread_index),
                                       *pg_comm->gpu_device_storage);
        {
            std::stringstream ss;
            native::details::adjacency_matrix matrix =
                    device_group_ring_topology::build_p2p_capability_matrix(ss,
                                                                            get_device_affinity(thread_index));
            output << ss.str() <<"\nResult matrix:\n" << matrix <<  std::endl;
            ss.clear();

            //check matrix
            bool res;
            std::string descr;
            std::tie(res, descr) = check_adj_matrix(matrix, get_device_affinity(thread_index),
                                                    DEVICE_GROUP_WEIGHT, DEVICE_GROUP_WEIGHT);
            UT_ASSERT(res == true, descr);
            output << descr << std::endl;

            std::tie(res, descr) = check_id_ring(matrix, get_device_affinity(thread_index),
                                                 native::details::plain_graph
                                                 {
                                                     ccl::device_index_type(0,0, ccl::unused_index_value),
                                                     ccl::device_index_type(0,1, ccl::unused_index_value),
                                                     ccl::device_index_type(0,2, ccl::unused_index_value)
                                                 });
            UT_ASSERT(res == true, descr);

/* built alredy -- skip
            res = top.build(ss, thread_index, get_device_affinity(thread_index), matrix);
            UT_ASSERT(res, "Cannot build topology: " << ss.str());
*/
            output << ss.str() << std::endl;

            native::details::printer<topology> p;
            ccl_tuple_for_each(*tg_comm->get_device_group_ctx(thread_index)->get_group_topology<topology>()->devices, p);
            output << "Router:\n" << p.to_string();

            //Check topology
            std::tie(res, descr) = check_topology<topology>(tg_comm->get_device_group_ctx(thread_index)->get_group_topology<topology>()->devices,
                                                            expected_indices_tuple({
                                                                                    optional_indices{true,{0, 1, 2}, {}}
                                                                                   }));
            UT_ASSERT(res, descr);
        }
    }


    {
        init_routing_data(process_index, 1, 1);
        auto thread_device_list = create_devices_by_affinity(thread_index,
                                                             {
                                                                 ccl::device_index_type(0,0, ccl::unused_index_value),
                                                                 ccl::device_index_type(0,2, ccl::unused_index_value),
                                                                 ccl::device_index_type(0,3, ccl::unused_index_value),
                                                                 ccl::device_index_type(0,2, ccl::unused_index_value)
                                                             });
        device_group_ring_topology top(*tg_comm->get_device_group_ctx(thread_index),
                                       *pg_comm->gpu_device_storage);
        {
            std::stringstream ss;
            native::details::adjacency_matrix matrix =
                    device_group_ring_topology::build_p2p_capability_matrix(ss,
                                                                            get_device_affinity(thread_index));
            output << ss.str() <<"\nResult matrix:\n" << matrix << std::endl;
            ss.clear();

            //check matrix
            bool res;
            std::string descr;
            std::tie(res, descr) = check_adj_matrix(matrix, get_device_affinity(thread_index),
                                                    DEVICE_GROUP_WEIGHT, DEVICE_GROUP_WEIGHT);
            UT_ASSERT(res == true, descr);
            output << descr << std::endl;

            std::tie(res, descr) = check_id_ring(matrix, get_device_affinity(thread_index),
                                                 native::details::plain_graph
                                                 {
                                                     ccl::device_index_type(0,0, ccl::unused_index_value),
                                                     ccl::device_index_type(0,2, ccl::unused_index_value),
                                                     ccl::device_index_type(0,2, ccl::unused_index_value),
                                                     ccl::device_index_type(0,3, ccl::unused_index_value)
                                                 });
            UT_ASSERT(res == true, descr);

/* built alredy -- skip
            res = top.build(ss, thread_index, get_device_affinity(thread_index), matrix);
            UT_ASSERT(res, "Cannot build topology: " << ss.str());
*/
            output << ss.str() << std::endl;

            native::details::printer<topology> p;
            ccl_tuple_for_each(*tg_comm->get_device_group_ctx(thread_index)->get_group_topology<topology>()->devices, p);
            output << "Router:\n" << p.to_string();

            //Check topology
            std::tie(res, descr) = check_topology<topology>(tg_comm->get_device_group_ctx(thread_index)->get_group_topology<topology>()->devices,
                                                            expected_indices_tuple({
                                                                                    optional_indices{true,{0, 1, 3}, {}},
                                                                                    optional_indices{true, {2}, {}}
                                                                                   }));
            UT_ASSERT(res, descr);
        }
    }
}


TEST_F(router_fixture, device_group_topology_scaleup_test)
{
    using namespace utils;
    using namespace native;
    using namespace native::details;

    size_t process_index = 0;
    size_t thread_index = 0;

    ccl::context_comm_addr comm_addr;
    comm_addr.thread_idx = thread_index;
    comm_addr.thread_count = 1;
    comm_addr.comm_rank = 0;
    comm_addr.comm_size = 1;

    constexpr ccl::device_topology_type topology = ccl::device_topology_type::device_group_torn_apart_ring;
    {
        output << "TEST: two inaccessible group with two devices" << std::endl;
        init_routing_data(process_index, 1, 1);
        fill_indices_data(thread_index,
                                        {
                                            ccl::device_index_type(0,0, ccl::unused_index_value),
                                            ccl::device_index_type(0,1, ccl::unused_index_value),
                                            ccl::device_index_type(0,2, ccl::unused_index_value),
                                            ccl::device_index_type(0,3, ccl::unused_index_value)
                                        });
        std::shared_ptr<device_group_context> group_ctx(new device_group_context(comm_addr,
                                                                                 get_device_affinity(thread_index)));
        device_group_ring_topology top(*group_ctx,
                                       *pg_comm->gpu_device_storage);
        {
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
                                            }
                                        };



            std::stringstream ss;
            adjacency_matrix matrix =
                    device_group_ring_topology::build_p2p_capability_matrix(
                                 ss, get_device_affinity(thread_index),
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
            res = top.build(ss, comm_addr, get_device_affinity(thread_index), matrix);
            output << ss.str() << std::endl;
            if (!res)
            {
                UT_ASSERT(res, "Cannot build topology: " << ss.str());
            }

            native::details::printer<topology> p;
            ccl_tuple_for_each(*group_ctx->get_group_topology<topology>()->devices, p);
            output << "Router:\n" << p.to_string();

            //Check topology
            std::tie(res, descr) = check_topology<topology>(group_ctx->get_group_topology<topology>()->devices,
                                                            expected_indices_tuple(
                                                                {
                                                                    optional_indices{true,{0, 2}, {0, 0}},
                                                                    optional_indices{false,{},{}},
                                                                    optional_indices{false,{},{}},
                                                                    optional_indices{false,{},{}},
                                                                    optional_indices{false,{},{}},
                                                                    optional_indices{false,{},{}},
                                                                    optional_indices{false,{},{}},
                                                                    optional_indices{true,{1, 3},{1, 1}},
                                                                    optional_indices{false,{},{}}
                                                                }));
            UT_ASSERT(res, descr);
        }
    }

    {
        output << "TEST: two inaccessible devices" << std::endl;
        init_routing_data(process_index, 1, 1);
        fill_indices_data(thread_index,
                                        {
                                            ccl::device_index_type(0,0, ccl::unused_index_value),
                                            ccl::device_index_type(0,1, ccl::unused_index_value)
                                        });
        std::shared_ptr<device_group_context> group_ctx(new device_group_context(comm_addr,
                                                                                 get_device_affinity(thread_index)));
        device_group_ring_topology top(*group_ctx,
                                       *pg_comm->gpu_device_storage);
        {
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
            adjacency_matrix matrix =
                    device_group_ring_topology::build_p2p_capability_matrix(
                                 ss, get_device_affinity(thread_index),
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
            res = top.build(ss, comm_addr, get_device_affinity(thread_index), matrix);
            output << ss.str() << std::endl;
            if (!res)
            {
                UT_ASSERT(res, "Cannot build topology: " << ss.str());
            }

            native::details::printer<topology> p;
            ccl_tuple_for_each(*group_ctx->get_group_topology<topology>()->devices, p);
            output << "Router:\n" << p.to_string();

            //Check topology
            std::tie(res, descr) = check_topology<topology>(group_ctx->get_group_topology<topology>()->devices,
                                                            expected_indices_tuple(
                                                                {
                                                                    optional_indices{false,{}, {}},
                                                                    optional_indices{false,{},{}},
                                                                    optional_indices{false,{},{}},
                                                                    optional_indices{false,{},{}},
                                                                    optional_indices{false,{},{}},
                                                                    optional_indices{false,{},{}},
                                                                    optional_indices{false,{},{}},
                                                                    optional_indices{true,{0, 1},{0, 0}},
                                                                    optional_indices{false,{},{}}
                                                                }));
            UT_ASSERT(res, descr);
        }
    }

    {
        output << "TEST: two inaccessible asymmetric groups" << std::endl;
        init_routing_data(process_index, 1, 1);
        fill_indices_data(thread_index,
                                        {
                                            ccl::device_index_type(0,0, ccl::unused_index_value),
                                            ccl::device_index_type(0,1, ccl::unused_index_value),
                                            ccl::device_index_type(0,2, ccl::unused_index_value),
                                            ccl::device_index_type(0,3, ccl::unused_index_value)
                                        });
        std::shared_ptr<device_group_context> group_ctx(new device_group_context(comm_addr,
                                                                                 get_device_affinity(thread_index)));
        device_group_ring_topology top(*group_ctx,
                                       *pg_comm->gpu_device_storage);
        {
            adjacency_matrix expected_matrix
                                        {
                                            {
                                                ccl::device_index_type(0,0, ccl::unused_index_value),
                                                {
                                                    {ccl::device_index_type(0,0, ccl::unused_index_value), 1},
                                                    {ccl::device_index_type(0,1, ccl::unused_index_value), 1},
                                                    {ccl::device_index_type(0,2, ccl::unused_index_value), 1},
                                                    {ccl::device_index_type(0,3, ccl::unused_index_value), 0}
                                                }
                                            },
                                            {
                                                ccl::device_index_type(0,1, ccl::unused_index_value),
                                                {
                                                    {ccl::device_index_type(0,0, ccl::unused_index_value), 1},
                                                    {ccl::device_index_type(0,1, ccl::unused_index_value), 1},
                                                    {ccl::device_index_type(0,2, ccl::unused_index_value), 1},
                                                    {ccl::device_index_type(0,3, ccl::unused_index_value), 0}
                                                }
                                            },
                                            {
                                                ccl::device_index_type(0,2, ccl::unused_index_value),
                                                {
                                                    {ccl::device_index_type(0,0, ccl::unused_index_value), 1},
                                                    {ccl::device_index_type(0,1, ccl::unused_index_value), 1},
                                                    {ccl::device_index_type(0,2, ccl::unused_index_value), 1},
                                                    {ccl::device_index_type(0,3, ccl::unused_index_value), 0}
                                                }
                                            },
                                            {
                                                ccl::device_index_type(0,3, ccl::unused_index_value),
                                                {
                                                    {ccl::device_index_type(0,0, ccl::unused_index_value), 0},
                                                    {ccl::device_index_type(0,1, ccl::unused_index_value), 0},
                                                    {ccl::device_index_type(0,2, ccl::unused_index_value), 0},
                                                    {ccl::device_index_type(0,3, ccl::unused_index_value), 1}
                                                }
                                            }
                                        };



            std::stringstream ss;
            adjacency_matrix matrix =
                    device_group_ring_topology::build_p2p_capability_matrix(
                                 ss, get_device_affinity(thread_index),
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
            res = top.build(ss, comm_addr, get_device_affinity(thread_index), matrix);
            output << ss.str() << std::endl;
            if (!res)
            {
                UT_ASSERT(res, "Cannot build topology: " << ss.str());
            }

            native::details::printer<topology> p;
            ccl_tuple_for_each(*group_ctx->get_group_topology<topology>()->devices, p);
            output << "Router:\n" << p.to_string();

            //Check topology
            std::tie(res, descr) = check_topology<topology>(group_ctx->get_group_topology<topology>()->devices,
                                                            expected_indices_tuple(
                                                                {
                                                                    optional_indices{true,{0,1}, {0,1}},
                                                                    optional_indices{false,{},{}},
                                                                    optional_indices{false,{},{}},
                                                                    optional_indices{false,{},{}},
                                                                    optional_indices{false,{},{}},
                                                                    optional_indices{false,{},{}},
                                                                    optional_indices{false,{},{}},
                                                                    optional_indices{true,{2, 3},{2, 0}},
                                                                    optional_indices{false,{},{}}
                                                                }));
            UT_ASSERT(res, descr);
        }
    }

    {
        output << "TEST: two inaccessible group with real & virtual devices" << std::endl;
        init_routing_data(process_index, 1, 1);
        fill_indices_data(thread_index,
                                        {
                                            ccl::device_index_type(0,0, ccl::unused_index_value),
                                            ccl::device_index_type(0,0, ccl::unused_index_value),
                                            ccl::device_index_type(0,1, ccl::unused_index_value),
                                            ccl::device_index_type(0,1, ccl::unused_index_value)
                                        });
        std::shared_ptr<device_group_context> group_ctx(new device_group_context(comm_addr,
                                                                                 get_device_affinity(thread_index)));
        device_group_ring_topology top(*group_ctx,
                                       *pg_comm->gpu_device_storage);
        {
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
            adjacency_matrix matrix =
                    device_group_ring_topology::build_p2p_capability_matrix(
                                 ss, get_device_affinity(thread_index),
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
            res = top.build(ss, comm_addr, get_device_affinity(thread_index), matrix);
            output << ss.str() << std::endl;
            if (!res)
            {
                UT_ASSERT(res, "Cannot build topology: " << ss.str());
            }

            native::details::printer<topology> p;
            ccl_tuple_for_each(*group_ctx->get_group_topology<topology>()->devices, p);
            output << "Router:\n" << p.to_string();

            //Check topology
            std::tie(res, descr) = check_topology<topology>(group_ctx->get_group_topology<topology>()->devices,
                                                            expected_indices_tuple(
                                                                {
                                                                    optional_indices{true,{0, 2}, {0, 0}},
                                                                    optional_indices{false,{},{}},
                                                                    optional_indices{false,{},{}},
                                                                    optional_indices{false,{},{}},
                                                                    optional_indices{false,{},{}},
                                                                    optional_indices{false,{},{}},
                                                                    optional_indices{false,{},{}},
                                                                    optional_indices{false,{},{}},
                                                                    optional_indices{true,{1, 3}, {1, 1}}
                                                                }));
            UT_ASSERT(res, descr);
        }
    }
}
}
