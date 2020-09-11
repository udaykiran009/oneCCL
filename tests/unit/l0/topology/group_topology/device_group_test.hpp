#pragma once
#include "../topology_utils.hpp"
#include "../../stubs/stub_platform.hpp"

namespace topology_suite {

TEST_F(router_fixture, real_virtual_topology_test) {
    using namespace utils;
    using namespace native;

    size_t process_index = 0;
    size_t thread_index = 0;

    constexpr ccl::device_group_split_type topology = ccl::device_group_split_type::thread;
    constexpr ccl::device_topology_type class_id = ccl::device_topology_type::ring;
    {
        init_routing_data(process_index);
        create_devices_by_affinity(thread_index,
                                   { ccl::device_index_type(0, 0, ccl::unused_index_value),
                                     ccl::device_index_type(0, 0, ccl::unused_index_value),
                                     ccl::device_index_type(0, 0, ccl::unused_index_value) });
        device_group_ring_topology top(*tg_comm->get_device_group_ctx(thread_index),
                                       *pg_comm->gpu_device_storage);
        std::stringstream ss;
        native::details::adjacency_matrix matrix =
            device_group_ring_topology::build_p2p_capability_matrix(
                ss, get_device_affinity(thread_index), all_p2p_accessible);
        output << ss.str() << "\nResult matrix:\n" << matrix << std::endl;
        ss.clear();

        //check matrix
        bool res;
        std::string descr;
        std::tie(res, descr) = check_adj_matrix(
            matrix, get_device_affinity(thread_index), DEVICE_GROUP_WEIGHT, DEVICE_GROUP_WEIGHT);
        UT_ASSERT(res == true, descr);

        std::tie(res, descr) = check_id_ring(
            matrix,
            get_device_affinity(thread_index),
            native::details::plain_graph{ ccl::device_index_type(0, 0, ccl::unused_index_value),
                                          ccl::device_index_type(0, 0, ccl::unused_index_value),
                                          ccl::device_index_type(0, 0, ccl::unused_index_value) });
        UT_ASSERT(res == true, descr);

        try {
            res = top.build(ss,
                            tg_comm->thread_device_group_ctx[thread_index]->context_addr,
                            get_device_affinity(thread_index),
                            matrix);
        }
        catch (const std::exception& ex) {
            res = false;
            ss << "Cannot build topology: " << ex.what();
        }

        if (!res) {
            UT_ASSERT(res, "Cannot build topology: " << ss.str());
        }
        output << ss.str() << std::endl;

        //Check topology
        std::tie(res, descr) = check_topology<topology>(
            tg_comm->get_device_group_ctx(thread_index)
                ->get_group_topology<class_id>()
                .get_topology(0)
                ->devices,
            expected_indices_tuple(
                { optional_indices{ true, { 0 }, {} }, optional_indices{ true, { 1, 2 }, {} } }));
        UT_ASSERT(res, descr);
    }
}
TEST_F(router_fixture, multiple_real_topology_test) {
    using namespace utils;
    using namespace native;

    size_t process_index = 0;
    size_t thread_index = 0;

    constexpr ccl::device_group_split_type topology = ccl::device_group_split_type::thread;
    constexpr ccl::device_topology_type class_id = ccl::device_topology_type::ring;

    {
        init_routing_data(process_index);
        create_devices_by_affinity(thread_index,
                                   { ccl::device_index_type(0, 0, ccl::unused_index_value),
                                     ccl::device_index_type(0, 1, ccl::unused_index_value),
                                     ccl::device_index_type(0, 2, ccl::unused_index_value) });
        device_group_ring_topology top(*tg_comm->get_device_group_ctx(thread_index),
                                       *pg_comm->gpu_device_storage);
        std::stringstream ss;
        native::details::adjacency_matrix matrix =
            device_group_ring_topology::build_p2p_capability_matrix(
                ss, get_device_affinity(thread_index), all_p2p_accessible);
        output << ss.str() << "\nResult matrix:\n" << matrix << std::endl;
        ss.clear();

        //check matrix
        bool res;
        std::string descr;
        std::tie(res, descr) = check_adj_matrix(
            matrix, get_device_affinity(thread_index), DEVICE_GROUP_WEIGHT, DEVICE_GROUP_WEIGHT);
        UT_ASSERT(res == true, descr);

        std::tie(res, descr) = check_id_ring(
            matrix,
            get_device_affinity(thread_index),
            native::details::plain_graph{ ccl::device_index_type(0, 0, ccl::unused_index_value),
                                          ccl::device_index_type(0, 1, ccl::unused_index_value),
                                          ccl::device_index_type(0, 2, ccl::unused_index_value) });
        UT_ASSERT(res == true, descr);

        try {
            res = top.build(ss,
                            tg_comm->thread_device_group_ctx[thread_index]->context_addr,
                            get_device_affinity(thread_index),
                            matrix);
        }
        catch (const std::exception& ex) {
            res = false;
            ss << "Cannot build topology: " << ex.what();
        }

        if (!res) {
            UT_ASSERT(res, "Cannot build topology: " << ss.str());
        }
        output << ss.str() << std::endl;

        //Check topology
        std::tie(res, descr) = check_topology<topology>(
            tg_comm->get_device_group_ctx(thread_index)
                ->get_group_topology<class_id>()
                .get_topology(0)
                ->devices,
            expected_indices_tuple({ optional_indices{ true, { 0, 1, 2 }, {} } }));
        UT_ASSERT(res, descr);
    }
}

TEST_F(router_fixture, multiple_real_virtual_topology_test) {
    using namespace utils;
    using namespace native;

    size_t process_index = 0;
    size_t thread_index = 0;

    constexpr ccl::device_group_split_type topology = ccl::device_group_split_type::thread;
    constexpr ccl::device_topology_type class_id = ccl::device_topology_type::ring;

    {
        init_routing_data(process_index);
        create_devices_by_affinity(thread_index,
                                   { ccl::device_index_type(0, 0, ccl::unused_index_value),
                                     ccl::device_index_type(0, 2, ccl::unused_index_value),
                                     ccl::device_index_type(0, 3, ccl::unused_index_value),
                                     ccl::device_index_type(0, 2, ccl::unused_index_value) });
        device_group_ring_topology top(*tg_comm->get_device_group_ctx(thread_index),
                                       *pg_comm->gpu_device_storage);
        {
            std::stringstream ss;
            native::details::adjacency_matrix matrix =
                device_group_ring_topology::build_p2p_capability_matrix(
                    ss, get_device_affinity(thread_index), all_p2p_accessible);
            output << ss.str() << "\nResult matrix:\n" << matrix << std::endl;
            ss.clear();

            //check matrix
            bool res;
            std::string descr;
            std::tie(res, descr) = check_adj_matrix(matrix,
                                                    get_device_affinity(thread_index),
                                                    DEVICE_GROUP_WEIGHT,
                                                    DEVICE_GROUP_WEIGHT);
            UT_ASSERT(res == true, descr);
            output << descr << std::endl;

            std::tie(res, descr) =
                check_id_ring(matrix,
                              get_device_affinity(thread_index),
                              native::details::plain_graph{
                                  ccl::device_index_type(0, 0, ccl::unused_index_value),
                                  ccl::device_index_type(0, 2, ccl::unused_index_value),
                                  ccl::device_index_type(0, 2, ccl::unused_index_value),
                                  ccl::device_index_type(0, 3, ccl::unused_index_value) });
            UT_ASSERT(res == true, descr);

            res = top.build(ss,
                            tg_comm->thread_device_group_ctx[thread_index]->context_addr,
                            get_device_affinity(thread_index),
                            matrix);
            UT_ASSERT(res, "Cannot build topology: " << ss.str());

            output << ss.str() << std::endl;

            //Check topology
            std::tie(res, descr) = check_topology<topology>(
                tg_comm->get_device_group_ctx(thread_index)
                    ->get_group_topology<class_id>()
                    .get_topology(0)
                    ->devices,
                expected_indices_tuple({ optional_indices{ true, { 0, 1, 3 }, {} },
                                         optional_indices{ true, { 2 }, {} } }));
            UT_ASSERT(res, descr);
        }
    }
}

TEST_F(router_fixture, two_inaccessible_group_scaleup_test) {
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

    constexpr ccl::device_group_split_type topology = ccl::device_group_split_type::thread;
    //device_group_torn_apart_ring;
    constexpr ccl::device_topology_type class_id = ccl::device_topology_type::ring;
    {
        init_routing_data(process_index);
        fill_indices_data(thread_index,
                          { ccl::device_index_type(0, 0, ccl::unused_index_value),
                            ccl::device_index_type(0, 1, ccl::unused_index_value),
                            ccl::device_index_type(0, 2, ccl::unused_index_value),
                            ccl::device_index_type(0, 3, ccl::unused_index_value) });
        std::shared_ptr<device_group_context> group_ctx(
            new device_group_context(comm_addr, get_device_affinity(thread_index)));
        device_group_ring_topology top(*group_ctx, *pg_comm->gpu_device_storage);
        {
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
            adjacency_matrix matrix = device_group_ring_topology::build_p2p_capability_matrix(
                ss,
                get_device_affinity(thread_index),
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
            res = top.build(ss, comm_addr, get_device_affinity(thread_index), matrix);
            output << ss.str() << std::endl;
            if (!res) {
                UT_ASSERT(res, "Cannot build topology: " << ss.str());
            }

            //Check topology
            std::tie(res, descr) = check_topology<topology>(
                group_ctx->get_group_topology<class_id>().get_additiona_topology(0)->devices,
                expected_indices_tuple({ optional_indices{ true, { 0, 2 }, { 0, 0 } },
                                         optional_indices{ false, {}, {} },
                                         optional_indices{ false, {}, {} },
                                         optional_indices{ false, {}, {} },
                                         optional_indices{ false, {}, {} },
                                         optional_indices{ false, {}, {} },
                                         optional_indices{ false, {}, {} },
                                         optional_indices{ true, { 1, 3 }, { 1, 1 } },
                                         optional_indices{ false, {}, {} } }));
            UT_ASSERT(res, descr);
        }
    }
}

TEST_F(router_fixture, two_inaccessible_devices_topology_test) {
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

    constexpr ccl::device_group_split_type topology = ccl::device_group_split_type::thread;
    //device_group_torn_apart_ring;
    constexpr ccl::device_topology_type class_id = ccl::device_topology_type::ring;
    {
        init_routing_data(process_index);
        fill_indices_data(thread_index,
                          { ccl::device_index_type(0, 0, ccl::unused_index_value),
                            ccl::device_index_type(0, 1, ccl::unused_index_value) });
        std::shared_ptr<device_group_context> group_ctx(
            new device_group_context(comm_addr, get_device_affinity(thread_index)));
        device_group_ring_topology top(*group_ctx, *pg_comm->gpu_device_storage);
        {
            adjacency_matrix expected_matrix{
                { ccl::device_index_type(0, 0, ccl::unused_index_value),
                  { { ccl::device_index_type(0, 0, ccl::unused_index_value), 1 },
                    { ccl::device_index_type(0, 1, ccl::unused_index_value), 0 } } },
                { ccl::device_index_type(0, 1, ccl::unused_index_value),
                  { { ccl::device_index_type(0, 0, ccl::unused_index_value), 0 },
                    { ccl::device_index_type(0, 1, ccl::unused_index_value), 1 } } }
            };

            std::stringstream ss;
            adjacency_matrix matrix = device_group_ring_topology::build_p2p_capability_matrix(
                ss,
                get_device_affinity(thread_index),
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
            res = top.build(ss, comm_addr, get_device_affinity(thread_index), matrix);
            output << ss.str() << std::endl;
            if (!res) {
                UT_ASSERT(res, "Cannot build topology: " << ss.str());
            }

            //Check topology
            std::tie(res, descr) = check_topology<topology>(
                group_ctx->get_group_topology<class_id>().get_additiona_topology(0)->devices,
                expected_indices_tuple({ optional_indices{ false, {}, {} },
                                         optional_indices{ false, {}, {} },
                                         optional_indices{ false, {}, {} },
                                         optional_indices{ false, {}, {} },
                                         optional_indices{ false, {}, {} },
                                         optional_indices{ false, {}, {} },
                                         optional_indices{ false, {}, {} },
                                         optional_indices{ true, { 0, 1 }, { 0, 0 } },
                                         optional_indices{ false, {}, {} } }));
            UT_ASSERT(res, descr);
        }
    }
}

TEST_F(router_fixture, two_inaccessible_asym_groups_topology_test) {
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

    constexpr ccl::device_group_split_type topology = ccl::device_group_split_type::thread;
    //device_group_torn_apart_ring;
    constexpr ccl::device_topology_type class_id = ccl::device_topology_type::ring;
    {
        init_routing_data(process_index);
        fill_indices_data(thread_index,
                          { ccl::device_index_type(0, 0, ccl::unused_index_value),
                            ccl::device_index_type(0, 1, ccl::unused_index_value),
                            ccl::device_index_type(0, 2, ccl::unused_index_value),
                            ccl::device_index_type(0, 3, ccl::unused_index_value) });
        std::shared_ptr<device_group_context> group_ctx(
            new device_group_context(comm_addr, get_device_affinity(thread_index)));
        device_group_ring_topology top(*group_ctx, *pg_comm->gpu_device_storage);
        {
            adjacency_matrix expected_matrix{
                { ccl::device_index_type(0, 0, ccl::unused_index_value),
                  { { ccl::device_index_type(0, 0, ccl::unused_index_value), 1 },
                    { ccl::device_index_type(0, 1, ccl::unused_index_value), 1 },
                    { ccl::device_index_type(0, 2, ccl::unused_index_value), 1 },
                    { ccl::device_index_type(0, 3, ccl::unused_index_value), 0 } } },
                { ccl::device_index_type(0, 1, ccl::unused_index_value),
                  { { ccl::device_index_type(0, 0, ccl::unused_index_value), 1 },
                    { ccl::device_index_type(0, 1, ccl::unused_index_value), 1 },
                    { ccl::device_index_type(0, 2, ccl::unused_index_value), 1 },
                    { ccl::device_index_type(0, 3, ccl::unused_index_value), 0 } } },
                { ccl::device_index_type(0, 2, ccl::unused_index_value),
                  { { ccl::device_index_type(0, 0, ccl::unused_index_value), 1 },
                    { ccl::device_index_type(0, 1, ccl::unused_index_value), 1 },
                    { ccl::device_index_type(0, 2, ccl::unused_index_value), 1 },
                    { ccl::device_index_type(0, 3, ccl::unused_index_value), 0 } } },
                { ccl::device_index_type(0, 3, ccl::unused_index_value),
                  { { ccl::device_index_type(0, 0, ccl::unused_index_value), 0 },
                    { ccl::device_index_type(0, 1, ccl::unused_index_value), 0 },
                    { ccl::device_index_type(0, 2, ccl::unused_index_value), 0 },
                    { ccl::device_index_type(0, 3, ccl::unused_index_value), 1 } } }
            };

            std::stringstream ss;
            adjacency_matrix matrix = device_group_ring_topology::build_p2p_capability_matrix(
                ss,
                get_device_affinity(thread_index),
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
            res = top.build(ss, comm_addr, get_device_affinity(thread_index), matrix);
            output << ss.str() << std::endl;
            if (!res) {
                UT_ASSERT(res, "Cannot build topology: " << ss.str());
            }

            //Check topology
            std::tie(res, descr) = check_topology<topology>(
                group_ctx->get_group_topology<class_id>().get_additiona_topology(0)->devices,
                expected_indices_tuple({ optional_indices{ true, { 0, 1 }, { 0, 1 } },
                                         optional_indices{ false, {}, {} },
                                         optional_indices{ false, {}, {} },
                                         optional_indices{ false, {}, {} },
                                         optional_indices{ false, {}, {} },
                                         optional_indices{ false, {}, {} },
                                         optional_indices{ false, {}, {} },
                                         optional_indices{ true, { 2, 3 }, { 2, 0 } },
                                         optional_indices{ false, {}, {} } }));
            UT_ASSERT(res, descr);
        }
    }
}
TEST_F(router_fixture, two_inaccessible_real_virtual_groups_topology_test) {
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

    constexpr ccl::device_group_split_type topology = ccl::device_group_split_type::thread;
    //device_group_torn_apart_ring;
    constexpr ccl::device_topology_type class_id = ccl::device_topology_type::ring;
    {
        init_routing_data(process_index);
        fill_indices_data(thread_index,
                          { ccl::device_index_type(0, 0, ccl::unused_index_value),
                            ccl::device_index_type(0, 0, ccl::unused_index_value),
                            ccl::device_index_type(0, 1, ccl::unused_index_value),
                            ccl::device_index_type(0, 1, ccl::unused_index_value) });
        std::shared_ptr<device_group_context> group_ctx(
            new device_group_context(comm_addr, get_device_affinity(thread_index)));
        device_group_ring_topology top(*group_ctx, *pg_comm->gpu_device_storage);
        {
            adjacency_matrix expected_matrix{
                { ccl::device_index_type(0, 0, ccl::unused_index_value),
                  { { ccl::device_index_type(0, 0, ccl::unused_index_value), 1 },
                    { ccl::device_index_type(0, 1, ccl::unused_index_value), 0 } } },
                { ccl::device_index_type(0, 1, ccl::unused_index_value),
                  { { ccl::device_index_type(0, 0, ccl::unused_index_value), 0 },
                    { ccl::device_index_type(0, 1, ccl::unused_index_value), 1 } } }
            };

            std::stringstream ss;
            adjacency_matrix matrix = device_group_ring_topology::build_p2p_capability_matrix(
                ss,
                get_device_affinity(thread_index),
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
            res = top.build(ss, comm_addr, get_device_affinity(thread_index), matrix);
            output << ss.str() << std::endl;
            if (!res) {
                UT_ASSERT(res, "Cannot build topology: " << ss.str());
            }

            //Check topology
            std::tie(res, descr) = check_topology<topology>(
                group_ctx->get_group_topology<class_id>().get_additiona_topology(0)->devices,
                expected_indices_tuple({ optional_indices{ true, { 0, 2 }, { 0, 0 } },
                                         optional_indices{ false, {}, {} },
                                         optional_indices{ false, {}, {} },
                                         optional_indices{ false, {}, {} },
                                         optional_indices{ false, {}, {} },
                                         optional_indices{ false, {}, {} },
                                         optional_indices{ false, {}, {} },
                                         optional_indices{ false, {}, {} },
                                         optional_indices{ true, { 1, 3 }, { 1, 1 } } }));
            UT_ASSERT(res, descr);
        }
    }
}
} // namespace topology_suite
