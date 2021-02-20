#pragma once
#include <initializer_list>
#include "../topology_utils.hpp"
#include "../mocs/process_group_ring_topology.hpp"

#include "process_group_ring_ipc_test.hpp"
#include "process_group_ring_numa_test.hpp"
#include "process_group_ring_scaleup_test.hpp"
#include "process_group_ring_scaleup_numa_test.hpp"

namespace processgroup_topology_suite {

TEST_F(router_fixture, process_group_test_single_process_single_threads_topology_test) {
    using namespace utils;
    using namespace native;

    size_t process_index = 0;
    constexpr ccl::group_split_type topology = ccl::group_split_type::cluster;
    {
        stub::make_stub_devices({ dev_0 });

        //emulate last thread barrier creation
        //prepare thread context
        process_creator_params params = prepare_process_params(
            process_index, { { 0, { dev_0, dev_0 } } }, { { process_index, { dev_0, dev_0 } } });
        allied_process_group_ring_topology top(process_index,
                                               params.thread_ids.size(),
                                               *pg_comm,
                                               *pg_comm->gpu_device_storage,
                                               params.cluster_device_rank_offset,
                                               params.cluster_device_size);

        bool res;
        std::string descr;
        std::stringstream ss;
        native::detail::adjacency_matrix matrix =
            allied_process_group_ring_topology::build_p2p_capability_matrix(
                ss, params.total_node_mask, ::utils::all_p2p_accessible);

        std::vector<ccl::device_indices_type> ipc_device_indices =
            process_group_context::get_ipc_device_indices_for_id(process_index,
                                                                 params.total_node_mask);
        UT_ASSERT(ipc_device_indices.empty(),
                  "ipc_device_indices should be empty for single node case");

        // stub for cluster device topology
        native::detail::global_sorted_colored_plain_graphs control_cluster_topology{
            { 0,
              {
                  { { 0, dev_0 }, { 0, dev_0 } },
              } },
        };
        pg_comm->set_collect_cluster_colored_plain_graphs(control_cluster_topology);

        // build device group context at first
        for (size_t thread_id : params.thread_ids) {
            const ccl::device_indices_type& group_indices_affinity = get_device_affinity(thread_id);
            device_group_context& dev_group_ctx = *tg_comm->get_device_group_ctx(thread_id);
            device_group_ring_topology device_top(dev_group_ctx, *pg_comm->gpu_device_storage);

            std::stringstream ss;
            native::detail::adjacency_matrix matrix =
                device_group_ring_topology::build_p2p_capability_matrix(
                    ss, group_indices_affinity, ::utils::all_p2p_accessible);
            device_top.build(ss, dev_group_ctx.context_addr, group_indices_affinity, matrix);
        }

        //test topology creation
        try {
            res =
                top.build_all(ss, tg_comm->per_thread_indices, matrix, ::utils::all_p2p_accessible);
            output << ss.str() << std::endl;
        }
        catch (const std::exception& ex) {
            output << ss.str() << std::endl;
            res = false;
            descr += std::string("\nfailed with exception: ") + ex.what();
        }

        UT_ASSERT(res, "Cannot build topology: " << descr);

        //Check topology
        std::vector<expected_indices_tuple> expected_thread_results;
        //thread 0
        set_control_indices<ccl_gpu_comm>(
            expected_thread_results, 0, optional_indices{ true, { 0 }, { 0 } });
        set_control_indices<ccl_virtual_gpu_comm>(
            expected_thread_results, 0, optional_indices{ true, { 1 }, { 1 } });

        std::tie(res, descr) = check_ring_multiple_topologies<topology,
                                                              ccl::device_topology_type::ring,
                                                              process_group_context>(
            pg_comm->process_device_topology, params.thread_ids, expected_thread_results);
        UT_ASSERT(res, descr);
    }
}

TEST_F(router_fixture,
       process_group_test_single_process_single_threads_single_device_topology_test) {
    using namespace utils;
    using namespace native;

    size_t process_index = 0;
    constexpr ccl::group_split_type topology = ccl::group_split_type::cluster;
    {
        stub::make_stub_devices({ dev_0 });

        //emulate last thread barrier creation
        //prepare thread context
        process_creator_params params =
            prepare_process_params(process_index,
                                   { { 0, { dev_0, dev_0, dev_0 } } },
                                   { { process_index, { dev_0, dev_0, dev_0 } } });
        allied_process_group_ring_topology top(process_index,
                                               params.thread_ids.size(),
                                               *pg_comm,
                                               *pg_comm->gpu_device_storage,
                                               params.cluster_device_rank_offset,
                                               params.cluster_device_size);

        bool res;
        std::string descr;
        std::stringstream ss;
        native::detail::adjacency_matrix matrix =
            allied_process_group_ring_topology::build_p2p_capability_matrix(
                ss, params.total_node_mask, ::utils::all_p2p_accessible);

        std::vector<ccl::device_indices_type> ipc_device_indices =
            process_group_context::get_ipc_device_indices_for_id(process_index,
                                                                 params.total_node_mask);
        UT_ASSERT(ipc_device_indices.empty(),
                  "ipc_device_indices should be empty for single node case");

        // stub for cluster device topology
        native::detail::global_sorted_colored_plain_graphs control_cluster_topology{
            { 0,
              {
                  { { 0, dev_0 }, { 0, dev_0 }, { 0, dev_0 } },
              } },
        };
        pg_comm->set_collect_cluster_colored_plain_graphs(control_cluster_topology);

        // build device group context at first
        for (size_t thread_id : params.thread_ids) {
            const ccl::device_indices_type& group_indices_affinity = get_device_affinity(thread_id);
            device_group_context& dev_group_ctx = *tg_comm->get_device_group_ctx(thread_id);
            device_group_ring_topology device_top(dev_group_ctx, *pg_comm->gpu_device_storage);

            std::stringstream ss;
            native::detail::adjacency_matrix matrix =
                device_group_ring_topology::build_p2p_capability_matrix(
                    ss, group_indices_affinity, ::utils::all_p2p_accessible);
            device_top.build(ss, dev_group_ctx.context_addr, group_indices_affinity, matrix);
        }

        //test topology creation
        try {
            res =
                top.build_all(ss, tg_comm->per_thread_indices, matrix, ::utils::all_p2p_accessible);
            output << ss.str() << std::endl;
        }
        catch (const std::exception& ex) {
            output << ss.str() << std::endl;
            res = false;
            descr += std::string("\nfailed with exception: ") + ex.what();
        }

        UT_ASSERT(res, "Cannot build topology: " << descr);

        //Check topology
        std::vector<expected_indices_tuple> expected_thread_results;
        //thread 0
        expected_thread_results.push_back(expected_indices_tuple(
            { optional_indices{ true, { 0 }, {} }, optional_indices{ true, { 1, 2 }, {} } }));
        std::tie(res, descr) = check_ring_multiple_topologies<topology,
                                                              ccl::device_topology_type::ring,
                                                              process_group_context>(
            pg_comm->process_device_topology, params.thread_ids, expected_thread_results);
        UT_ASSERT(res, descr);
    }
}

TEST_F(router_fixture, process_group_test_ally_process_group_topology_test) {
    using namespace utils;
    using namespace native;

    size_t process_index = 0;
    constexpr ccl::group_split_type topology = ccl::group_split_type::cluster;
    {
        stub::make_stub_devices({ dev_0 });

        //emulate last thread barrier creation
        process_creator_params params =
            prepare_process_params(process_index,
                                   { { 0, { dev_0 } }, { 1, { dev_0 } } },
                                   { { process_index, { dev_0, dev_0 } } });

        allied_process_group_ring_topology top(process_index,
                                               params.process_ids.size(),
                                               *pg_comm,
                                               *pg_comm->gpu_device_storage,
                                               params.cluster_device_rank_offset,
                                               params.cluster_device_size);

        std::stringstream ss;
        native::detail::adjacency_matrix matrix =
            allied_process_group_ring_topology::build_p2p_capability_matrix(
                ss, params.total_node_mask, ::utils::all_p2p_accessible);
        output << ss.str() << "\nResult matrix:\n" << matrix << std::endl;
        ss.clear();

        std::vector<ccl::device_indices_type> ipc_device_indices =
            process_group_context::get_ipc_device_indices_for_id(process_index,
                                                                 params.total_node_mask);
        UT_ASSERT(ipc_device_indices.empty(),
                  "ipc_device_indices should be empty for single node case");

        bool res;
        std::string descr;

        // stub for cluster device topology
        native::detail::global_sorted_colored_plain_graphs control_cluster_topology{
            { 0,
              {
                  { { 0, dev_0 }, { 0, dev_0 } },
              } },
        };
        pg_comm->set_collect_cluster_colored_plain_graphs(control_cluster_topology);

        // build device group context at first
        for (size_t thread_id : params.thread_ids) {
            const ccl::device_indices_type& group_indices_affinity = get_device_affinity(thread_id);
            device_group_context& dev_group_ctx = *tg_comm->get_device_group_ctx(thread_id);
            device_group_ring_topology device_top(dev_group_ctx, *pg_comm->gpu_device_storage);

            std::stringstream ss;
            native::detail::adjacency_matrix matrix =
                device_group_ring_topology::build_p2p_capability_matrix(
                    ss, group_indices_affinity, ::utils::all_p2p_accessible);
            device_top.build(ss, dev_group_ctx.context_addr, group_indices_affinity, matrix);
        }

        res = top.build_all(ss, tg_comm->per_thread_indices, matrix, ::utils::all_p2p_accessible);
        UT_ASSERT(res, "Cannot build topology: " << ss.str());
        output << ss.str() << std::endl;

        //Check topology
        std::vector<expected_indices_tuple> expected_thread_results;
        //thread 0
        expected_thread_results.push_back(
            expected_indices_tuple({ optional_indices{ true, { 0 }, {} },
                                     optional_indices{ false, {}, {} },
                                     optional_indices{ false, {}, {} },
                                     optional_indices{ true, { 1 }, {} } }));
        //thread 1
        expected_thread_results.push_back(
            expected_indices_tuple({ optional_indices{ false, {}, {} },
                                     optional_indices{ true, { 1 }, {} },
                                     optional_indices{ true, { 0 }, {} } }));

        std::tie(res, descr) = check_ring_multiple_topologies<topology,
                                                              ccl::device_topology_type::ring,
                                                              process_group_context>(
            pg_comm->process_device_topology, params.thread_ids, expected_thread_results);
        UT_ASSERT(res, descr);
    }

    {
        stub::make_stub_devices({ dev_0, dev_1, dev_2, dev_3 });
        //emulate last thread barrier creation
        process_creator_params params = prepare_process_params(
            process_index,
            { { 0, { dev_0, dev_0, dev_0 } }, { 1, { dev_1, dev_1, dev_2 } } },
            { { process_index, { dev_0, dev_0, dev_0, dev_1, dev_1, dev_2 } }, { 1, { dev_3 } } });

        ::unit_test::allied_process_group_ring_topology_mock top(process_index,
                                                                 params.process_ids.size(),
                                                                 *pg_comm,
                                                                 *pg_comm->gpu_device_storage,
                                                                 params.cluster_device_rank_offset,
                                                                 params.cluster_device_size);

        std::stringstream ss;
        native::detail::adjacency_matrix matrix =
            allied_process_group_ring_topology::build_p2p_capability_matrix(
                ss, params.total_node_mask, ::utils::all_p2p_accessible);
        output << ss.str() << "\nResult matrix:\n" << matrix << std::endl;
        ss.clear();

        std::vector<ccl::device_indices_type> ipc_device_indices =
            process_group_context::get_ipc_device_indices_for_id(process_index,
                                                                 params.total_node_mask);
        UT_ASSERT(ipc_device_indices.size() == 1, "one ipc_device_indices should be");
        UT_ASSERT(ipc_device_indices[0].size() == 1, "one ipc_device_indices should be");

        bool res;
        std::string descr;

        //MockData
        // stub for cluster device topology
        native::detail::global_sorted_colored_plain_graphs control_cluster_topology{
            { 0,
              {
                  { { 0, dev_0 },
                    { 0, dev_0 },
                    { 0, dev_0 },
                    { 0, dev_1 },
                    { 0, dev_1 },
                    { 0, dev_2 } },
              } },
            { 1,
              {
                  { { 1, dev_3 } },
              } },
        };
        pg_comm->set_collect_cluster_colored_plain_graphs(control_cluster_topology);

        top.set_mock_colored_cluster_graphs_to_merge({ { process_index,
                                                         { { { process_index, dev_0 },
                                                             { process_index, dev_0 },
                                                             { process_index, dev_0 },
                                                             { process_index, dev_1 },
                                                             { process_index, dev_1 },
                                                             { process_index, dev_2 } } } },
                                                       { 1, { { { 1, dev_3 } } } } });
        top.set_mock_merged_colored_cluster_graphs({ { process_index,
                                                       { { { 2, dev_3 },
                                                           { process_index, dev_0 },
                                                           { process_index, dev_0 },
                                                           { process_index, dev_0 },
                                                           { process_index, dev_1 },
                                                           { process_index, dev_1 },
                                                           { process_index, dev_2 },
                                                           { 1, dev_3 } } } },
                                                     { 1,
                                                       { {
                                                           { process_index, dev_2 },
                                                           { 1, dev_3 },
                                                           { 2, dev_0 },
                                                       } } } });

        // build device group context at first
        for (size_t thread_id : params.thread_ids) {
            const ccl::device_indices_type& group_indices_affinity = get_device_affinity(thread_id);
            device_group_context& dev_group_ctx = *tg_comm->get_device_group_ctx(thread_id);
            device_group_ring_topology device_top(dev_group_ctx, *pg_comm->gpu_device_storage);

            std::stringstream ss;
            native::detail::adjacency_matrix matrix =
                device_group_ring_topology::build_p2p_capability_matrix(
                    ss, group_indices_affinity, ::utils::all_p2p_accessible);
            device_top.build(ss, dev_group_ctx.context_addr, group_indices_affinity, matrix);
        }

        try {
            res =
                top.build_all(ss, tg_comm->per_thread_indices, matrix, ::utils::all_p2p_accessible);
            output << ss.str() << std::endl;
        }
        catch (const std::exception& ex) {
            output << ss.str() << std::endl;
            res = false;
            descr += std::string("\nfailed with exception: ") + ex.what();
        }

        UT_ASSERT(res, "Cannot build topology: " << descr);

        //Check topology
        std::vector<expected_indices_tuple> expected_thread_results;
        //thread 0
        expected_thread_results.push_back(expected_indices_tuple({
            optional_indices{ false, {}, {} },
            optional_indices{ true, { 1, 2 }, {} },
            optional_indices{ true, { 3 }, {} },
            optional_indices{ false, {}, {} },
            optional_indices{ true, { 0 }, {} },
            optional_indices{ false, { 0 }, {} },
        }));
        //thread 1
        expected_thread_results.push_back(
            expected_indices_tuple({ optional_indices{ true, { 3, 5 }, {} },
                                     optional_indices{ true, { 4 }, {} },
                                     optional_indices{ false, {}, {} },
                                     optional_indices{ false, {}, {} },
                                     optional_indices{ false, {}, {} },
                                     optional_indices{ false, {}, {} },
                                     optional_indices{ true, { 6 }, {} } }));

        std::tie(res, descr) = check_ring_multiple_topologies<topology,
                                                              ccl::device_topology_type::ring,
                                                              process_group_context>(
            pg_comm->process_device_topology, params.thread_ids, expected_thread_results);
        UT_ASSERT(res, descr);
    }

    {
        //emulate last thread barrier creation
        process_creator_params params = prepare_process_params(
            process_index,
            { { 0, { dev_0, dev_2, dev_0 } }, { 1, { dev_1, dev_2, dev_0 } } },
            { { 0, { dev_0, dev_2, dev_0, dev_1, dev_2, dev_0 } },
              { 1, { dev_3, dev_1, dev_2 } } });

        ::unit_test::allied_process_group_ring_topology_mock top(process_index,
                                                                 params.process_ids.size(),
                                                                 *pg_comm,
                                                                 *pg_comm->gpu_device_storage,
                                                                 params.cluster_device_rank_offset,
                                                                 params.cluster_device_size);

        std::stringstream ss;
        native::detail::adjacency_matrix matrix =
            allied_process_group_ring_topology::build_p2p_capability_matrix(
                ss, params.total_node_mask, ::utils::all_p2p_accessible);
        output << ss.str() << "\nResult matrix:\n" << matrix << std::endl;
        ss.clear();

        std::vector<ccl::device_indices_type> ipc_device_indices =
            process_group_context::get_ipc_device_indices_for_id(process_index,
                                                                 params.total_node_mask);
        UT_ASSERT(ipc_device_indices.size() == 1, "three ipc_device_indices should be");
        UT_ASSERT(ipc_device_indices[0].size() == 3, "three ipc_device_indices should be");

        bool res;
        std::string descr;

        //MockData
        // stub for cluster device topology
        native::detail::global_sorted_colored_plain_graphs control_cluster_topology{
            { 0,
              { {
                  { 0, dev_0 },
                  { 0, dev_2 },
                  { 0, dev_0 },
                  { 0, dev_1 },
                  { 0, dev_2 },
                  { 0, dev_0 },
              } } },
            { 1,
              {
                  { { 1, dev_1 }, { 1, dev_2 }, { 1, dev_3 } },
              } },
        };
        pg_comm->set_collect_cluster_colored_plain_graphs(control_cluster_topology);

        top.set_mock_colored_cluster_graphs_to_merge(
            { { process_index,
                { {
                    { process_index, dev_0 },
                    { process_index, dev_2 },
                    { process_index, dev_0 },
                    { process_index, dev_1 },
                    { process_index, dev_2 },
                    { process_index, dev_0 },
                } } },
              { 1, { { { 1, dev_1 }, { 1, dev_2 }, { 1, dev_3 } } } } });
        top.set_mock_merged_colored_cluster_graphs({ { process_index,
                                                       { { { 2, dev_3 },
                                                           { process_index, dev_0 },
                                                           { process_index, dev_0 },
                                                           { process_index, dev_0 },
                                                           { process_index, dev_1 },
                                                           { process_index, dev_1 },
                                                           { process_index, dev_2 },
                                                           { 1, dev_1 } } } },
                                                     { 1,
                                                       { {
                                                           { process_index, dev_2 },
                                                           { 1, dev_1 },
                                                           { 1, dev_2 },
                                                           { 1, dev_3 },
                                                           { 2, dev_0 },
                                                       } } } });

        // build device group context at first
        for (size_t thread_id : params.thread_ids) {
            const ccl::device_indices_type& group_indices_affinity = get_device_affinity(thread_id);
            device_group_context& dev_group_ctx = *tg_comm->get_device_group_ctx(thread_id);
            device_group_ring_topology device_top(dev_group_ctx, *pg_comm->gpu_device_storage);

            std::stringstream ss;
            native::detail::adjacency_matrix matrix =
                device_group_ring_topology::build_p2p_capability_matrix(
                    ss, group_indices_affinity, ::utils::all_p2p_accessible);
            device_top.build(ss, dev_group_ctx.context_addr, group_indices_affinity, matrix);
        }

        try {
            res =
                top.build_all(ss, tg_comm->per_thread_indices, matrix, ::utils::all_p2p_accessible);
            output << ss.str() << std::endl;
        }
        catch (const std::exception& ex) {
            output << ss.str() << std::endl;
            res = false;
            descr += std::string("\nfailed with exception: ") + ex.what();
        }

        UT_ASSERT(res, "Cannot build topology: " << descr);

        //Check topology
        std::vector<expected_indices_tuple> expected_thread_results;
        set_control_indices<ccl_ipc_source_gpu_comm<ccl_gpu_comm>>(
            expected_thread_results, 0, optional_indices{ true, { 0 }, { 0 } });
        set_control_indices<ccl_gpu_comm>(
            expected_thread_results, 0, optional_indices{ true, { 1 }, { 1 } });
        set_control_indices<ccl_virtual_gpu_comm>(
            expected_thread_results, 0, optional_indices{ true, { 2 }, { 2 } });
        set_control_indices<ccl_thread_comm<ccl_gpu_comm>>(
            expected_thread_results, 0, optional_indices{ true, { 3 }, { 3 } });

        set_control_indices<ccl_gpu_comm>(
            expected_thread_results, 1, optional_indices{ true, { 3 }, { 3 } });
        set_control_indices<ccl_virtual_gpu_comm>(
            expected_thread_results, 1, optional_indices{ true, { 4, 5 }, { 4, 5 } });
        set_control_indices<ccl_ipc_gpu_comm>(
            expected_thread_results, 1, optional_indices{ true, { 6 }, { 6 } });

        std::tie(res, descr) = check_ring_multiple_topologies<topology,
                                                              ccl::device_topology_type::ring,
                                                              process_group_context>(
            pg_comm->process_device_topology, params.thread_ids, expected_thread_results, false);
        UT_ASSERT(res, descr);
    }
}

TEST_F(router_fixture,
       process_group_test_several_processes_with_inner_numa_in_process_group_topology_test) {
    using namespace utils;
    using namespace native;
    using namespace native::detail;

    size_t process_index = 1;

    constexpr ccl::group_split_type topology = ccl::group_split_type::cluster;
    {
        process_creator_params params =
            prepare_process_params(process_index,
                                   { { 0, { dev_0, dev_1 } }, { 1, { dev_2, dev_3 } } },
                                   { { 0, { dev_0, dev_1 } },
                                     { process_index, { dev_0, dev_1, dev_2, dev_3 } },
                                     { 2, { dev_2, dev_3 } } });

        adjacency_matrix expected_matrix{
            { dev_0, { { dev_0, 1 }, { dev_1, 1 }, { dev_2, 0 }, { dev_3, 0 } } },
            { dev_1, { { dev_0, 1 }, { dev_1, 1 }, { dev_2, 0 }, { dev_3, 0 } } },
            { dev_2, { { dev_0, 0 }, { dev_1, 0 }, { dev_2, 1 }, { dev_3, 1 } } },
            { dev_3, { { dev_0, 0 }, { dev_1, 0 }, { dev_2, 1 }, { dev_3, 1 } } }
        };

        ::unit_test::allied_process_group_ring_topology_mock top(process_index,
                                                                 params.process_ids.size(),
                                                                 *pg_comm,
                                                                 *pg_comm->gpu_device_storage,
                                                                 params.cluster_device_rank_offset,
                                                                 params.cluster_device_size);

        std::stringstream ss;
        adjacency_matrix matrix = allied_process_group_ring_topology::build_p2p_capability_matrix(
            ss,
            params.total_node_mask,
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

        std::vector<ccl::device_indices_type> ipc_device_indices =
            process_group_context::get_ipc_device_indices_for_id(process_index,
                                                                 params.total_node_mask);
        UT_ASSERT(!ipc_device_indices.empty(),
                  "ipc_device_indices should not be empty for single node case");

        bool res;
        std::string descr;

        // Mock data
        // stub for whole cluster device topology
        native::detail::global_sorted_colored_plain_graphs control_cluster_topology{
            { 0,
              { {
                  { 0, dev_0 },
                  { 0, dev_1 },
              } } },
            { process_index,
              { {
                    { process_index, dev_0 },
                    { process_index, dev_1 },
                },
                {
                    { process_index, dev_2 },
                    { process_index, dev_3 },
                } } },
            { 2,
              { {
                  { 2, dev_2 },
                  { 2, dev_3 },
              } } },
        };
        pg_comm->set_collect_cluster_colored_plain_graphs(control_cluster_topology);

        top.set_mock_colored_cluster_graphs_to_merge({
            //to_check
            { 0, { { { 0, dev_0 }, { 0, dev_1 } } } },

            { process_index, //in terms of communicator our process_index is` zero`
              { { { process_index, ccl::device_index_type(0, 0, ccl::unused_index_value) },
                  { process_index, ccl::device_index_type(0, 1, ccl::unused_index_value) } },
                { { process_index, ccl::device_index_type(0, 2, ccl::unused_index_value) },
                  { process_index, ccl::device_index_type(0, 3, ccl::unused_index_value) } } } },
            { 2, { { { 2, dev_2 }, { 2, dev_3 } } } },
        });

        top.set_mock_merged_colored_cluster_graphs(
            { //to_check
              { process_index,
                {
                    { { 0, ccl::device_index_type(0, 0, ccl::unused_index_value) },
                      { 0, ccl::device_index_type(0, 1, ccl::unused_index_value) },
                      { process_index, ccl::device_index_type(0, 0, ccl::unused_index_value) },
                      { process_index, ccl::device_index_type(0, 1, ccl::unused_index_value) } },
                    { { process_index, ccl::device_index_type(0, 2, ccl::unused_index_value) },
                      { process_index, ccl::device_index_type(0, 3, ccl::unused_index_value) },
                      { 2, ccl::device_index_type(0, 2, ccl::unused_index_value) },
                      { 2, ccl::device_index_type(0, 3, ccl::unused_index_value) } },
                } } },
            { //to_replace
              { 0,
                {
                    { { 0, ccl::device_index_type(0, 0, ccl::unused_index_value) },
                      { 0, ccl::device_index_type(0, 1, ccl::unused_index_value) },
                      { process_index, ccl::device_index_type(0, 0, ccl::unused_index_value) },
                      { process_index, ccl::device_index_type(0, 1, ccl::unused_index_value) } },
                } },

              { process_index,
                {
                    { { 0, ccl::device_index_type(0, 0, ccl::unused_index_value) },
                      { 0, ccl::device_index_type(0, 1, ccl::unused_index_value) },
                      { process_index, ccl::device_index_type(0, 0, ccl::unused_index_value) },
                      { process_index, ccl::device_index_type(0, 1, ccl::unused_index_value) } },
                    { { process_index, ccl::device_index_type(0, 2, ccl::unused_index_value) },
                      { process_index, ccl::device_index_type(0, 3, ccl::unused_index_value) },
                      { 2, ccl::device_index_type(0, 2, ccl::unused_index_value) },
                      { 2, ccl::device_index_type(0, 3, ccl::unused_index_value) } },
                } },
              { 2,
                {
                    { { process_index, ccl::device_index_type(0, 2, ccl::unused_index_value) },
                      { process_index, ccl::device_index_type(0, 3, ccl::unused_index_value) },
                      { 2, ccl::device_index_type(0, 2, ccl::unused_index_value) },
                      { 2, ccl::device_index_type(0, 3, ccl::unused_index_value) } },
                } } });

        // build device group context at first
        for (size_t thread_id : params.thread_ids) {
            const ccl::device_indices_type& group_indices_affinity = get_device_affinity(thread_id);
            device_group_context& dev_group_ctx = *tg_comm->get_device_group_ctx(thread_id);
            device_group_ring_topology device_top(dev_group_ctx, *pg_comm->gpu_device_storage);

            std::stringstream ss;
            native::detail::adjacency_matrix matrix =
                device_group_ring_topology::build_p2p_capability_matrix(
                    ss, group_indices_affinity, ::utils::all_p2p_accessible);
            device_top.build(ss, dev_group_ctx.context_addr, group_indices_affinity, matrix);
        }

        try {
            res = top.build_all(ss,
                                tg_comm->per_thread_indices,
                                matrix,
                                std::bind(test_custom_p2p_ping,
                                          std::placeholders::_1,
                                          std::placeholders::_2,
                                          expected_matrix));
        }
        catch (const std::exception& ex) {
            res = false;
            descr += std::string("\nfailed with exception: ") + ex.what();
        }

        if (!res) {
            output << ss.str() << std::endl;
            UT_ASSERT(res, "Cannot build topology: " << descr);
        }

        //Check topology
        output << "\n\n\n" << ss.str() << std::endl;
        std::vector<expected_indices_tuple> expected_thread_results;
        set_control_indices<ccl_ipc_source_gpu_comm<ccl_gpu_comm>>(
            expected_thread_results, 0, optional_indices{ true, { 2 }, { 2 } });
        set_control_indices<ccl_numa_proxy<ccl_gpu_comm>>(
            expected_thread_results, 0, optional_indices{ true, { 3 }, { 3 } });

        set_control_indices<ccl_gpu_comm>(
            expected_thread_results, 1, optional_indices{ true, { 4 }, { 0 } });
        set_control_indices<ccl_numa_proxy<ccl_gpu_comm>>(
            expected_thread_results, 1, optional_indices{ true, { 5 }, { 1 } });
        set_control_indices<ccl_ipc_gpu_comm>(
            expected_thread_results, 1, optional_indices{ true, { 6 }, { 2 } });
        std::tie(res, descr) = check_ring_multiple_topologies<topology,
                                                              ccl::device_topology_type::ring,
                                                              process_group_context>(
            pg_comm->process_device_topology, params.thread_ids, expected_thread_results, true, 0);
        if (!res) {
            output << "\nLOG:\n" << ss.str() << std::endl;
            output << descr;
            UT_ASSERT(res, descr);
        }
    }
}

TEST_F(router_fixture, process_group_test_scale_up_scale_out_process_group_topology_test) {
    using namespace utils;
    using namespace native;
    using namespace native::detail;

    size_t process_index = 0;

    constexpr ccl::group_split_type topology = ccl::group_split_type::cluster;
    {
        output << "TEST: scaleup between thread groups in one process group" << std::endl;

        process_creator_params params = prepare_process_params(
            process_index,
            { { 0, { dev_0, dev_1 } }, { 1, { dev_2, dev_3 } } },

            { { process_index, { dev_0, dev_1, dev_2, dev_3 } }, { 1, { dev_4 } } });

        ::unit_test::allied_process_group_ring_topology_mock top(process_index,
                                                                 params.process_ids.size(),
                                                                 *pg_comm,
                                                                 *pg_comm->gpu_device_storage,
                                                                 params.cluster_device_rank_offset,
                                                                 params.cluster_device_size);
        adjacency_matrix expected_matrix{
            { dev_0, { { dev_0, 1 }, { dev_1, 1 }, { dev_2, 0 }, { dev_3, 0 }, { dev_4, 0 } } },
            { dev_1, { { dev_0, 1 }, { dev_1, 1 }, { dev_2, 0 }, { dev_3, 0 }, { dev_4, 0 } } },
            { dev_2, { { dev_0, 0 }, { dev_1, 0 }, { dev_2, 1 }, { dev_3, 1 }, { dev_4, 0 } } },
            { dev_3, { { dev_0, 0 }, { dev_1, 0 }, { dev_2, 1 }, { dev_3, 1 }, { dev_4, 0 } } },
            { dev_4, { { dev_0, 0 }, { dev_1, 0 }, { dev_2, 0 }, { dev_3, 0 }, { dev_4, 1 } } }
        };
        std::stringstream ss;
        adjacency_matrix matrix = allied_process_group_ring_topology::build_p2p_capability_matrix(
            ss,
            params.total_node_mask,
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

        std::vector<ccl::device_indices_type> ipc_device_indices =
            process_group_context::get_ipc_device_indices_for_id(process_index,
                                                                 params.total_node_mask);
        UT_ASSERT(ipc_device_indices[0].size() == 1, "one ipc_device_index");

        bool res;
        std::string descr;

        // Mock data
        // stub for whole cluster device topology
        native::detail::global_sorted_colored_plain_graphs control_cluster_topology{
            { process_index,
              { {
                    { process_index, dev_0 },
                    { process_index, dev_1 },
                },
                {
                    { process_index, dev_2 },
                    { process_index, dev_3 },
                } } },
            { 1,
              { {
                  { 1, dev_4 },
              } } },
        };
        pg_comm->set_collect_cluster_colored_plain_graphs(control_cluster_topology);
        top.set_mock_colored_cluster_graphs_to_merge({
            //to_check
            { process_index,
              {
                  { { process_index, dev_0 }, { process_index, dev_1 } },
                  { { process_index, dev_2 }, { process_index, dev_3 } },
              } },

            { 1,
              {
                  { { 1, dev_4 } },
              } },
        });

        top.set_mock_merged_colored_cluster_graphs(
            { //to_check
              { process_index,
                { { { process_index, dev_0 }, { process_index, dev_1 } },
                  { { process_index, dev_2 }, { process_index, dev_3 } } } } },
            { //to_replace
              { process_index,
                { { { process_index, dev_0 }, { process_index, dev_1 } },
                  { { process_index, dev_2 }, { process_index, dev_3 } } } },

              { 1, { { { 1, dev_4 } } } } });

        // build device group context at first
        for (size_t thread_id : params.thread_ids) {
            const ccl::device_indices_type& group_indices_affinity = get_device_affinity(thread_id);
            device_group_context& dev_group_ctx = *tg_comm->get_device_group_ctx(thread_id);
            device_group_ring_topology device_top(dev_group_ctx, *pg_comm->gpu_device_storage);

            std::stringstream ss;
            native::detail::adjacency_matrix matrix =
                device_group_ring_topology::build_p2p_capability_matrix(
                    ss, group_indices_affinity, ::utils::all_p2p_accessible);
            device_top.build(ss, dev_group_ctx.context_addr, group_indices_affinity, matrix);
        }

        try {
            res = top.build_all(ss,
                                tg_comm->per_thread_indices,
                                matrix,
                                std::bind(test_custom_p2p_ping,
                                          std::placeholders::_1,
                                          std::placeholders::_2,
                                          expected_matrix));
        }
        catch (const std::exception& ex) {
            res = false;
            descr += std::string("\nfailed with exception: ") + ex.what();
        }

        if (!res) {
            output << ss.str() << std::endl;
            UT_ASSERT(res, "Cannot build topology: " << descr);
        }

        output << "\n\n\n" << ss.str() << std::endl;
        //Check topology
        std::vector<expected_indices_tuple> expected_thread_results;
        set_control_indices<ccl_gpu_comm>(
            expected_thread_results, 0, optional_indices{ true, { 0 }, { 0 } });
        set_control_indices<ccl_numa_proxy<ccl_gpu_comm>>(
            expected_thread_results, 0, optional_indices{ true, { 1 }, { 1 } });
        set_control_indices<ccl_scaleout_proxy<ccl_gpu_comm>>(
            expected_thread_results, 1, optional_indices{ true, { 2 }, { 0 } });
        set_control_indices<ccl_numa_proxy<ccl_gpu_comm>>(
            expected_thread_results, 1, optional_indices{ true, { 3 }, { 1 } });
        std::tie(res, descr) = check_ring_multiple_topologies<topology,
                                                              ccl::device_topology_type::ring,
                                                              process_group_context>(
            pg_comm->process_device_topology, params.thread_ids, expected_thread_results, true, 0);
        UT_ASSERT(res, descr);
    }
}
} // namespace processgroup_topology_suite
