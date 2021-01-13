#pragma once
#include <initializer_list>
#include "../topology_utils.hpp"
#include "../mocs/process_group_ring_topology.hpp"

namespace processgroup_topology_suite {
//TODO SYMMETRIC tests - UNIFY IT
TEST_F(router_fixture, IPC_TEST_real_ats_2tile_for_process_0) {
    using namespace utils;
    using namespace native;
    using namespace native::detail;

    size_t process_index = 0;
    constexpr ccl::group_split_type topology = ccl::group_split_type::cluster;
    {
        stub::make_stub_devices({ { dev_0, dev_1 } });

        //emulate last thread barrier creation
        //prepare thread context
        process_creator_params params = prepare_process_params(0, //local process
                                                               { { 0,
                                                                   {
                                                                       dev_0,
                                                                   } } },
                                                               { //cluster
                                                                 { 0, //process 0
                                                                   {
                                                                       dev_0,
                                                                   } },
                                                                 { 1, //process 1
                                                                   {
                                                                       dev_1,
                                                                   } } });
        ::unit_test::allied_process_group_ring_topology_mock top(process_index,
                                                                 params.process_ids.size(),
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
        UT_ASSERT(!ipc_device_indices.empty(),
                  "ipc_device_indices should not be empty for single node case");

        // stub for cluster device topology
        native::detail::global_sorted_colored_plain_graphs control_cluster_topology{
            { 0,
              {
                  {
                      { 0, dev_0 },
                  },
              } },
            { 1,
              {
                  {
                      { 1, dev_1 },
                  },
              } }
        };
        pg_comm->set_collect_cluster_colored_plain_graphs(control_cluster_topology);
        top.set_mock_colored_cluster_graphs_to_merge({ { process_index,
                                                         { {
                                                             { process_index, dev_0 },
                                                         } } },
                                                       { 1, { { { 1, dev_1 } } } } });
        top.set_mock_merged_colored_cluster_graphs(
            { { process_index, { { { 2, dev_1 }, { process_index, dev_0 }, { 1, dev_1 } } } },
              { 1,
                { {
                    { process_index, dev_0 },
                    { 1, dev_1 },
                    { 2, dev_0 },
                } } } });

        // build device group context & thread group context at first
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
        std::vector<expected_indices_tuple> expected_seq;
        //thread 0
        set_control_indices<ccl_ipc_source_gpu_comm<ccl_gpu_comm>>(
            expected_seq, 0, optional_indices{ true, { 0 }, { 0 } });
        set_control_indices<ccl_ipc_gpu_comm>(expected_seq,
                                              0,
                                              optional_indices{ true,
                                                                { 1 },
                                                                {
                                                                    1,
                                                                } });
        std::tie(res, descr) = check_ring_multiple_topologies<topology,
                                                              ccl::device_topology_type::ring,
                                                              process_group_context>(
            pg_comm->process_device_topology, params.thread_ids, expected_seq);
        UT_ASSERT(res, descr);
    }
}

//TODO SYMMETRIC tests - UNIFY IT
TEST_F(router_fixture, IPC_TEST_real_ats_2tile_IPC_for_process_1) {
    using namespace utils;
    using namespace native;
    using namespace native::detail;

    size_t process_index = 1;
    constexpr ccl::group_split_type topology = ccl::group_split_type::cluster;
    {
        stub::make_stub_devices({ { dev_0, dev_1 } });

        //emulate last thread barrier creation
        //prepare thread context
        process_creator_params params = prepare_process_params(process_index, //local process
                                                               { { 0,
                                                                   {
                                                                       dev_1,
                                                                   } } },
                                                               { //cluster
                                                                 { 0, //process 0
                                                                   {
                                                                       dev_0,
                                                                   } },
                                                                 { 1, //process 1
                                                                   {
                                                                       dev_1,
                                                                   } } });
        ::unit_test::allied_process_group_ring_topology_mock top(process_index,
                                                                 params.process_ids.size(),
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
        UT_ASSERT(!ipc_device_indices.empty(),
                  "ipc_device_indices should not be empty for single node case");

        // stub for cluster device topology
        native::detail::global_sorted_colored_plain_graphs control_cluster_topology{
            { 0,
              {
                  {
                      { 0, dev_0 },
                  },
              } },
            { 1,
              {
                  {
                      { 1, dev_1 },
                  },
              } }
        };
        pg_comm->set_collect_cluster_colored_plain_graphs(control_cluster_topology);
        top.set_mock_colored_cluster_graphs_to_merge(
            { { 0,
                { {
                    { 0, dev_0 },
                } } },
              { process_index, { { { process_index, dev_1 } } } } });
        top.set_mock_merged_colored_cluster_graphs(
            { { 0, { { { 2, dev_1 }, { 0, dev_0 }, { 1, dev_1 } } } },
              { process_index,
                { {
                    { 0, dev_0 },
                    { process_index, dev_1 },
                    { 2, dev_0 },
                } } } });

        // build device group context & thread group context at first
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
        std::vector<expected_indices_tuple> expected_seq;
        //thread 0
        set_control_indices<ccl_ipc_source_gpu_comm<ccl_gpu_comm>>(
            expected_seq, 0, optional_indices{ true, { 1 }, { 1 } });
        set_control_indices<ccl_ipc_gpu_comm>(expected_seq,
                                              0,
                                              optional_indices{ true,
                                                                { 0 },
                                                                {
                                                                    0,
                                                                } });
        std::tie(res, descr) = check_ring_multiple_topologies<topology,
                                                              ccl::device_topology_type::ring,
                                                              process_group_context>(
            pg_comm->process_device_topology, params.thread_ids, expected_seq);
        UT_ASSERT(res, descr);
    }
}

} // namespace processgroup_topology_suite
