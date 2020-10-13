#pragma once
#include "../topology_utils.hpp"

namespace topology_suite {

TEST_F(router_fixture, thread_group_topology_test) {
    using namespace utils;
    using namespace native;

    size_t process_index = 0;
    std::vector<size_t> thread_ids{ 0, 1 };

    constexpr ccl::group_split_type topology = ccl::group_split_type::process;
    constexpr ccl::device_topology_type class_id = ccl::device_topology_type::ring;

    init_routing_data(process_index);
    {
        //emulate last thread barrier creation
        //prepare context
        tg_comm->per_thread_indices = ccl::process_device_indices_t{
            { 0,
              { ccl::device_index_type(0, 0, ccl::unused_index_value),
                ccl::device_index_type(0, 0, ccl::unused_index_value),
                ccl::device_index_type(0, 0, ccl::unused_index_value) } },
            { 1,
              { ccl::device_index_type(0, 1, ccl::unused_index_value),
                ccl::device_index_type(0, 1, ccl::unused_index_value),
                ccl::device_index_type(0, 2, ccl::unused_index_value) } }
        };
        native::detail::p2p_rating_function rating_function = all_p2p_accessible;
        for (size_t thread_id : thread_ids) {
            create_devices_by_affinity(thread_id,
                                       tg_comm->per_thread_indices.find(thread_id)->second);

            const ccl::device_indices_t& group_indices_affinity = get_device_affinity(thread_id);
            device_group_context& dev_group_ctx = *tg_comm->get_device_group_ctx(thread_id);
            device_group_ring_topology device_top(dev_group_ctx, *pg_comm->gpu_device_storage);

            std::stringstream ss;
            native::detail::adjacency_matrix matrix =
                device_group_ring_topology::build_p2p_capability_matrix(
                    ss, group_indices_affinity, rating_function);
            device_top.build(ss, dev_group_ctx.context_addr, group_indices_affinity, matrix);
        }
        // fill_thread_ctx(ctx);

        thread_group_ring_topology top(*tg_comm, *pg_comm->gpu_device_storage);
        std::stringstream ss;
        native::detail::adjacency_matrix matrix =
            thread_group_ring_topology::build_p2p_capability_matrix(
                ss, tg_comm->per_thread_indices, rating_function);
        output << ss.str() << "\nResult matrix:\n" << matrix << std::endl;
        ss.clear();

        bool res;
        std::string descr;

        //test device id ring creation
        std::vector<native::detail::plain_graph> expected_ring_ids;
        //thread 0
        expected_ring_ids.push_back({ ccl::device_index_type(0, 0, ccl::unused_index_value),
                                      ccl::device_index_type(0, 0, ccl::unused_index_value),
                                      ccl::device_index_type(0, 0, ccl::unused_index_value) });
        //thread 1
        expected_ring_ids.push_back({ ccl::device_index_type(0, 1, ccl::unused_index_value),
                                      ccl::device_index_type(0, 1, ccl::unused_index_value),
                                      ccl::device_index_type(0, 2, ccl::unused_index_value) });
        for (size_t thread_id : thread_ids) {
            std::tie(res, descr) = check_adj_matrix(
                matrix, get_device_affinity(thread_id), DEVICE_GROUP_WEIGHT, THREAD_GROUP_WEIGHT);
            if (!res) {
                output << "Thread: " << thread_id << ", check_adj_matrix log: " << descr
                       << std::endl;
                UT_ASSERT(res == true, descr);
            }

            std::tie(res, descr) =
                check_id_ring(matrix, get_device_affinity(thread_id), expected_ring_ids[thread_id]);
            if (!res) {
                output << "Thread: " << thread_id << ", check_id_ring log: " << descr << std::endl;
                UT_ASSERT(res == true, descr);
            }
        }

        try {
            ccl::context_comm_addr comm_addr;
            res = top.build(ss, comm_addr, tg_comm->per_thread_indices, matrix, rating_function);
        }
        catch (const std::exception& ex) {
            res = false;
            ss << "Cannot build topology: " << ex.what();
        }

        if (!res) {
            UT_ASSERT(res, "Cannot build topology: " << ss.str());
        }

        //Check topology
        std::vector<expected_indices_tuple> expected_thread_results;
        //thread 0
        expected_thread_results.push_back(
            expected_indices_tuple({ optional_indices{ true, { 0 }, {} },
                                     optional_indices{ true, { 1, 2 }, {} },
                                     optional_indices{ true, { 3 }, {} } }));
        //thread 1
        expected_thread_results.push_back(
            expected_indices_tuple({ optional_indices{ true, { 3, 5 }, {} },
                                     optional_indices{ true, { 4 }, {} },
                                     optional_indices{ true, { 0 }, {} } }));
        std::tie(res, descr) =
            check_ring_multiple_topologies<topology, class_id, thread_group_context>(
                tg_comm->thread_device_topology, thread_ids, expected_thread_results);
        UT_ASSERT(res, descr);
    }
}

TEST_F(router_fixture, same_device_topology_test) {
    using namespace utils;
    using namespace native;

    size_t process_index = 0;
    std::vector<size_t> thread_ids{ 0, 1 };

    constexpr ccl::group_split_type topology = ccl::group_split_type::process;
    constexpr ccl::device_topology_type class_id = ccl::device_topology_type::ring;

    //same device test
    init_routing_data(process_index);
    {
        //emulate last thread barrier creation
        //prepare context
        tg_comm->per_thread_indices = ccl::process_device_indices_t{
            { 0,
              { ccl::device_index_type(0, 0, ccl::unused_index_value),
                ccl::device_index_type(0, 0, ccl::unused_index_value),
                ccl::device_index_type(0, 0, ccl::unused_index_value) } },
            { 1,
              { ccl::device_index_type(0, 0, ccl::unused_index_value),
                ccl::device_index_type(0, 0, ccl::unused_index_value),
                ccl::device_index_type(0, 0, ccl::unused_index_value) } }
        };

        native::detail::p2p_rating_function rating_function = all_p2p_accessible;
        for (size_t thread_id : thread_ids) {
            create_devices_by_affinity(thread_id,
                                       tg_comm->per_thread_indices.find(thread_id)->second);

            const ccl::device_indices_t& group_indices_affinity = get_device_affinity(thread_id);
            device_group_context& dev_group_ctx = *tg_comm->get_device_group_ctx(thread_id);
            device_group_ring_topology device_top(dev_group_ctx, *pg_comm->gpu_device_storage);

            std::stringstream ss;
            native::detail::adjacency_matrix matrix =
                device_group_ring_topology::build_p2p_capability_matrix(
                    ss, group_indices_affinity, rating_function);
            device_top.build(ss, dev_group_ctx.context_addr, group_indices_affinity, matrix);
        }

        thread_group_ring_topology top(*tg_comm, *pg_comm->gpu_device_storage);
        std::stringstream ss;
        native::detail::adjacency_matrix matrix =
            thread_group_ring_topology::build_p2p_capability_matrix(
                ss, tg_comm->per_thread_indices, rating_function);
        output << ss.str() << "\nResult matrix:\n" << matrix << std::endl;
        ss.clear();

        bool res;
        std::string descr;

        //test device id ring creation
        std::vector<native::detail::plain_graph> expected_ring_ids;
        //thread 0
        expected_ring_ids.push_back({ ccl::device_index_type(0, 0, ccl::unused_index_value),
                                      ccl::device_index_type(0, 0, ccl::unused_index_value),
                                      ccl::device_index_type(0, 0, ccl::unused_index_value) });
        //thread 1
        expected_ring_ids.push_back({ ccl::device_index_type(0, 0, ccl::unused_index_value),
                                      ccl::device_index_type(0, 0, ccl::unused_index_value),
                                      ccl::device_index_type(0, 0, ccl::unused_index_value) });

        for (size_t thread_id : thread_ids) {
            std::tie(res, descr) = check_adj_matrix(
                matrix, get_device_affinity(thread_id), DEVICE_GROUP_WEIGHT, THREAD_GROUP_WEIGHT);
            UT_ASSERT(res == true, descr);
            output << "Thread: " << thread_id << ", check_adj_matrix log: " << descr << std::endl;

            std::tie(res, descr) =
                check_id_ring(matrix, get_device_affinity(thread_id), expected_ring_ids[thread_id]);
            UT_ASSERT(res == true, descr);
            output << "Thread: " << thread_id << ", check_id_ring log: " << descr << std::endl;
        }

        //test topology creation
        try {
            ccl::context_comm_addr comm_addr;
            res = top.build(ss, comm_addr, tg_comm->per_thread_indices, matrix, rating_function);
        }
        catch (const std::exception& ex) {
            res = false;
            ss << "Cannot build topology: " << ex.what();
        }

        if (!res) {
            UT_ASSERT(res, "Cannot build topology: " << ss.str());
        }

        //Check topology
        std::vector<expected_indices_tuple> expected_thread_results;

        //thread 0
        expected_thread_results.push_back(expected_indices_tuple(
            { optional_indices{ true, { 0 }, {} }, optional_indices{ true, { 1, 2 }, {} } }));
        //thread 1
        expected_thread_results.push_back(
            expected_indices_tuple({ optional_indices{ false, {}, {} },
                                     optional_indices{ true, { 3, 4, 5 }, {} },
                                     optional_indices{ true, { 0 }, {} } }));
        std::tie(res, descr) =
            check_ring_multiple_topologies<topology, class_id, thread_group_context>(
                tg_comm->thread_device_topology, thread_ids, expected_thread_results);
        UT_ASSERT(res, descr);
    }
}

TEST_F(router_fixture, thread_group_topology_scaleup_test) {
    using namespace utils;
    using namespace native;
    using namespace native::detail;

    size_t process_index = 0;
    std::vector<size_t> thread_ids{ 0, 1 };

    constexpr ccl::group_split_type topology = ccl::group_split_type::process;
    constexpr ccl::device_topology_type class_id = ccl::device_topology_type::ring;

    init_routing_data(process_index);
    {
        output << "TEST: two inaccessible devices for two threads" << std::endl;
        //emulate last thread barrier creation
        //prepare context
        tg_comm->per_thread_indices = ccl::process_device_indices_t{
            { 0,
              {
                  ccl::device_index_type(0, 0, ccl::unused_index_value),
              } },
            { 1,
              {
                  ccl::device_index_type(0, 1, ccl::unused_index_value),
              } }
        };

        adjacency_matrix expected_matrix{
            { ccl::device_index_type(0, 0, ccl::unused_index_value),
              { { ccl::device_index_type(0, 0, ccl::unused_index_value), 1 },
                { ccl::device_index_type(0, 1, ccl::unused_index_value), 0 } } },
            { ccl::device_index_type(0, 1, ccl::unused_index_value),
              { { ccl::device_index_type(0, 0, ccl::unused_index_value), 0 },
                { ccl::device_index_type(0, 1, ccl::unused_index_value), 1 } } }
        };

        native::detail::p2p_rating_function rating_function = std::bind(
            test_custom_p2p_ping, std::placeholders::_1, std::placeholders::_2, expected_matrix);
        for (size_t thread_id : thread_ids) {
            create_devices_by_affinity(thread_id,
                                       tg_comm->per_thread_indices.find(thread_id)->second);

            const ccl::device_indices_t& group_indices_affinity = get_device_affinity(thread_id);
            device_group_context& dev_group_ctx = *tg_comm->get_device_group_ctx(thread_id);
            device_group_ring_topology device_top(dev_group_ctx, *pg_comm->gpu_device_storage);

            std::stringstream ss;
            native::detail::adjacency_matrix matrix =
                device_group_ring_topology::build_p2p_capability_matrix(
                    ss, group_indices_affinity, rating_function);
            device_top.build(ss, dev_group_ctx.context_addr, group_indices_affinity, matrix);
        }

        std::stringstream ss;
        thread_group_ring_topology top(*tg_comm, *pg_comm->gpu_device_storage);
        native::detail::adjacency_matrix matrix =
            thread_group_ring_topology::build_p2p_capability_matrix(
                ss, tg_comm->per_thread_indices, rating_function);
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
        try {
            ccl::context_comm_addr comm_addr;
            res = top.build(ss, comm_addr, tg_comm->per_thread_indices, matrix, rating_function);
        }
        catch (const std::exception& ex) {
            res = false;
            ss << "Cannot build topology: " << ex.what();
        }

        if (!res) {
            UT_ASSERT(res, "Cannot build topology: " << ss.str());
        }

        //Check topology
        std::vector<expected_indices_tuple> expected_thread_results;
        //thread 0
        expected_thread_results.push_back(
            expected_indices_tuple({ optional_indices{ false, {}, {} },
                                     optional_indices{ false, {}, {} },
                                     optional_indices{ false, {}, {} },
                                     optional_indices{ false, {}, {} },
                                     optional_indices{ false, {}, {} },
                                     optional_indices{ false, {}, {} },
                                     optional_indices{ false, {}, {} },
                                     optional_indices{ true, { 0 }, { 0 } },
                                     optional_indices{ false, {}, {} } }));
        //thread 1
        expected_thread_results.push_back(
            expected_indices_tuple({ optional_indices{ false, {}, {} },
                                     optional_indices{ false, {}, {} },
                                     optional_indices{ false, {}, {} },
                                     optional_indices{ false, {}, {} },
                                     optional_indices{ false, {}, {} },
                                     optional_indices{ false, {}, {} },
                                     optional_indices{ false, {}, {} },
                                     optional_indices{ true, { 1 }, { 0 } },
                                     optional_indices{ false, {}, {} } }));
        std::tie(res, descr) =
            check_ring_multiple_topologies<topology, class_id, thread_group_context>(
                tg_comm->thread_device_topology, thread_ids, expected_thread_results, true);
        UT_ASSERT(res, descr);
    }
}

TEST_F(router_fixture, two_inaccessible_groups_topology_scaleup_test) {
    using namespace utils;
    using namespace native;
    using namespace native::detail;

    size_t process_index = 0;
    std::vector<size_t> thread_ids{ 0, 1 };

    constexpr ccl::group_split_type topology = ccl::group_split_type::process;
    constexpr ccl::device_topology_type class_id = ccl::device_topology_type::ring;

    init_routing_data(process_index);
    {
        //emulate last thread barrier creation
        //prepare context
        tg_comm->per_thread_indices = ccl::process_device_indices_t{
            { 0,
              { ccl::device_index_type(0, 0, ccl::unused_index_value),
                ccl::device_index_type(0, 1, ccl::unused_index_value) } },
            { 1,
              { ccl::device_index_type(0, 2, ccl::unused_index_value),
                ccl::device_index_type(0, 3, ccl::unused_index_value) } }
        };

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
                { ccl::device_index_type(0, 3, ccl::unused_index_value), 1 } } },
        };

        native::detail::p2p_rating_function rating_function = std::bind(
            test_custom_p2p_ping, std::placeholders::_1, std::placeholders::_2, expected_matrix);
        for (size_t thread_id : thread_ids) {
            create_devices_by_affinity(thread_id,
                                       tg_comm->per_thread_indices.find(thread_id)->second);

            const ccl::device_indices_t& group_indices_affinity = get_device_affinity(thread_id);
            device_group_context& dev_group_ctx = *tg_comm->get_device_group_ctx(thread_id);
            device_group_ring_topology device_top(dev_group_ctx, *pg_comm->gpu_device_storage);

            std::stringstream ss;
            native::detail::adjacency_matrix matrix =
                device_group_ring_topology::build_p2p_capability_matrix(
                    ss, group_indices_affinity, rating_function);
            device_top.build(ss, dev_group_ctx.context_addr, group_indices_affinity, matrix);
        }

        std::stringstream ss;
        thread_group_ring_topology top(*tg_comm, *pg_comm->gpu_device_storage);
        native::detail::adjacency_matrix matrix =
            thread_group_ring_topology::build_p2p_capability_matrix(
                ss, tg_comm->per_thread_indices, rating_function);
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
        try {
            ccl::context_comm_addr comm_addr;
            res = top.build(ss, comm_addr, tg_comm->per_thread_indices, matrix, rating_function);
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
        std::vector<expected_indices_tuple> expected_thread_results;
        //thread 0
        expected_thread_results.push_back(
            expected_indices_tuple({ optional_indices{ true, { 0 }, { 0 } },
                                     optional_indices{ false, {}, {} },
                                     optional_indices{ false, {}, {} },
                                     optional_indices{ false, {}, {} },
                                     optional_indices{ false, {}, {} },
                                     optional_indices{ false, {}, {} },
                                     optional_indices{ false, {}, {} },
                                     optional_indices{ true, { 1 }, { 1 } },
                                     optional_indices{ false, {}, {} } }));
        //thread 1
        expected_thread_results.push_back(
            expected_indices_tuple({ optional_indices{ true, { 2 }, { 0 } },
                                     optional_indices{ false, {}, {} },
                                     optional_indices{ false, {}, {} },
                                     optional_indices{ false, {}, {} },
                                     optional_indices{ false, {}, {} },
                                     optional_indices{ false, {}, {} },
                                     optional_indices{ false, {}, {} },
                                     optional_indices{ true, { 3 }, { 1 } },
                                     optional_indices{ false, {}, {} } }));
        std::tie(res, descr) =
            check_ring_multiple_topologies<topology, class_id, thread_group_context>(
                tg_comm->thread_device_topology, thread_ids, expected_thread_results, true);
        UT_ASSERT(res, descr);
    }
}

TEST_F(router_fixture, local_group_inaccessible_devices_topology_scaleup_test) {
    using namespace utils;
    using namespace native;
    using namespace native::detail;

    size_t process_index = 0;
    std::vector<size_t> thread_ids{ 0, 1 };

    constexpr ccl::group_split_type topology = ccl::group_split_type::process;
    constexpr ccl::device_topology_type class_id = ccl::device_topology_type::ring;

    init_routing_data(process_index);
    {
        output << "TEST: local group inaccessible devices to merge for two threads" << std::endl;
        //emulate last thread barrier creation
        //prepare context
        tg_comm->per_thread_indices = ccl::process_device_indices_t{
            { 0,
              {
                  ccl::device_index_type(0, 0, ccl::unused_index_value),
                  ccl::device_index_type(0, 1, ccl::unused_index_value),
                  ccl::device_index_type(0, 2, ccl::unused_index_value),
              } },
            { 1,
              {
                  ccl::device_index_type(0, 0, ccl::unused_index_value),
                  ccl::device_index_type(0, 1, ccl::unused_index_value),
                  ccl::device_index_type(0, 2, ccl::unused_index_value),
              } }
        };

        adjacency_matrix expected_matrix{
            { ccl::device_index_type(0, 0, ccl::unused_index_value),
              { { ccl::device_index_type(0, 0, ccl::unused_index_value), 1 },
                { ccl::device_index_type(0, 1, ccl::unused_index_value), 0 },
                { ccl::device_index_type(0, 2, ccl::unused_index_value), 1 } } },
            { ccl::device_index_type(0, 1, ccl::unused_index_value),
              { { ccl::device_index_type(0, 0, ccl::unused_index_value), 0 },
                { ccl::device_index_type(0, 1, ccl::unused_index_value), 1 },
                { ccl::device_index_type(0, 2, ccl::unused_index_value), 0 } } },
            { ccl::device_index_type(0, 2, ccl::unused_index_value),
              { { ccl::device_index_type(0, 0, ccl::unused_index_value), 1 },
                { ccl::device_index_type(0, 1, ccl::unused_index_value), 0 },
                { ccl::device_index_type(0, 2, ccl::unused_index_value), 1 } } },
        };

        native::detail::p2p_rating_function rating_function = std::bind(
            test_custom_p2p_ping, std::placeholders::_1, std::placeholders::_2, expected_matrix);
        for (size_t thread_id : thread_ids) {
            create_devices_by_affinity(thread_id,
                                       tg_comm->per_thread_indices.find(thread_id)->second);

            const ccl::device_indices_t& group_indices_affinity = get_device_affinity(thread_id);
            device_group_context& dev_group_ctx = *tg_comm->get_device_group_ctx(thread_id);
            device_group_ring_topology device_top(dev_group_ctx, *pg_comm->gpu_device_storage);

            //std::stringstream ss;
            native::detail::adjacency_matrix matrix =
                device_group_ring_topology::build_p2p_capability_matrix(
                    std::cout, group_indices_affinity, rating_function);
            device_top.build(std::cout, dev_group_ctx.context_addr, group_indices_affinity, matrix);
        }

        //std::stringstream ss;
        thread_group_ring_topology top(*tg_comm, *pg_comm->gpu_device_storage);
        native::detail::adjacency_matrix matrix =
            thread_group_ring_topology::build_p2p_capability_matrix(
                output, tg_comm->per_thread_indices, rating_function);
        //check matrix
        if (matrix != expected_matrix) {
            /*output << ss.str() << "\nResult initial_matrix:\n"
                   << matrix
                   << "\nExpected matrix:\n"
                   << expected_matrix << std::endl;*/
            UT_ASSERT(false, "Matrix should be equal to expected_matrix");
        }
        //ss.clear();

        bool res;
        std::string descr;
        try {
            ccl::context_comm_addr comm_addr;
            res =
                top.build(output, comm_addr, tg_comm->per_thread_indices, matrix, rating_function);
        }
        catch (const std::exception& ex) {
            res = false;
            output << "Cannot build topology: " << ex.what();
        }

        if (!res) {
            UT_ASSERT(res, "thread topology creation failed");
        }
        //output << ss.str() << std::endl;

        //Check topology
        std::vector<expected_indices_tuple> expected_thread_results;
        //thread 0
        expected_thread_results.push_back(
            expected_indices_tuple({ optional_indices{ true, { 0 }, { 0 } },
                                     optional_indices{ false, {}, {} },
                                     optional_indices{ false, {}, {} },
                                     optional_indices{ true, { 2, 5 }, { 2, 1 } },
                                     optional_indices{ false, {}, {} },
                                     optional_indices{ false, {}, {} },
                                     optional_indices{ false, {}, {} },
                                     optional_indices{ true, { 1, 4 }, { 1, 0 } },
                                     optional_indices{ false, {}, {} } }));
        //thread 1
        expected_thread_results.push_back(
            expected_indices_tuple({ optional_indices{ false, {}, {} },
                                     optional_indices{ true, { 2, 3, 5 }, { 2, 3, 1 } },
                                     optional_indices{ true, { 0 }, { 0 } },
                                     optional_indices{ false, {}, {} },
                                     optional_indices{ false, {}, {} },
                                     optional_indices{ false, {}, {} },
                                     optional_indices{ false, {}, {} },
                                     optional_indices{ false, {}, {} },
                                     optional_indices{ false, {}, {} } }));
        std::tie(res, descr) =
            check_ring_multiple_topologies<topology, class_id, thread_group_context>(
                tg_comm->thread_device_topology, thread_ids, expected_thread_results, true);
        UT_ASSERT(res, descr);
    }
}

} // namespace topology_suite
