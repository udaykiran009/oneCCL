#pragma once
#include <initializer_list>
#include "../topology_utils.hpp"
#include "../mocs/process_group_ring_topology.hpp"

namespace processgroup_topology_suite {
//TODO SYMMETRIC tests - UNIFY IT

const native::detail::adjacency_matrix& get_3_process_8_devices_matrix() {
    static const native::detail::adjacency_matrix matrix{ { dev_0,
                                                            { { dev_0, 1 },
                                                              { dev_1, 1 },
                                                              { dev_2, 0 },
                                                              { dev_3, 0 },
                                                              { dev_4, 0 },
                                                              { dev_5, 0 },
                                                              { dev_6, 0 },
                                                              { dev_7, 0 } } },
                                                          { dev_1,
                                                            { { dev_0, 1 },
                                                              { dev_1, 1 },
                                                              { dev_2, 0 },
                                                              { dev_3, 0 },
                                                              { dev_4, 0 },
                                                              { dev_5, 0 },
                                                              { dev_6, 0 },
                                                              { dev_7, 0 } } },
                                                          { dev_2,
                                                            { { dev_0, 0 },
                                                              { dev_1, 0 },
                                                              { dev_2, 1 },
                                                              { dev_3, 1 },
                                                              { dev_4, 0 },
                                                              { dev_5, 0 },
                                                              { dev_6, 0 },
                                                              { dev_7, 0 } } },
                                                          { dev_3,
                                                            { { dev_0, 0 },
                                                              { dev_1, 0 },
                                                              { dev_2, 1 },
                                                              { dev_3, 1 },
                                                              { dev_4, 0 },
                                                              { dev_5, 0 },
                                                              { dev_6, 0 },
                                                              { dev_7, 0 } } },
                                                          { dev_4,
                                                            { { dev_0, 0 },
                                                              { dev_1, 0 },
                                                              { dev_2, 0 },
                                                              { dev_3, 0 },
                                                              { dev_4, 1 },
                                                              { dev_5, 1 },
                                                              { dev_6, 0 },
                                                              { dev_7, 0 } } },
                                                          { dev_5,
                                                            { { dev_0, 0 },
                                                              { dev_1, 0 },
                                                              { dev_2, 0 },
                                                              { dev_3, 0 },
                                                              { dev_4, 1 },
                                                              { dev_5, 1 },
                                                              { dev_6, 0 },
                                                              { dev_7, 0 } } },
                                                          { dev_6,
                                                            { { dev_0, 0 },
                                                              { dev_1, 0 },
                                                              { dev_2, 0 },
                                                              { dev_3, 0 },
                                                              { dev_4, 0 },
                                                              { dev_5, 0 },
                                                              { dev_6, 1 },
                                                              { dev_7, 1 } } },
                                                          { dev_7,
                                                            { { dev_0, 0 },
                                                              { dev_1, 0 },
                                                              { dev_2, 0 },
                                                              { dev_3, 0 },
                                                              { dev_4, 0 },
                                                              { dev_5, 0 },
                                                              { dev_6, 1 },
                                                              { dev_7, 1 } } } };

    return matrix;
}
TEST_F(router_fixture, SCALEUP_NUMA_TEST_3_process_8_devices_process_0) {
    using namespace utils;
    using namespace native;
    using namespace native::detail;

    size_t process_index = 0;

    constexpr ccl::group_split_type topology = ccl::group_split_type::cluster;
    {
        process_creator_params params =
            prepare_process_params(process_index,
                                   { { 0, { dev_0, dev_1 } }, { 1, { dev_2, dev_3 } } },

                                   { { process_index, { dev_0, dev_1, dev_2, dev_3 } },
                                     { 1, { dev_4, dev_5 } },
                                     { 2, { dev_6, dev_7 } } });

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
                      get_3_process_8_devices_matrix()));
        //check matrix
        if (matrix != get_3_process_8_devices_matrix()) {
            output << ss.str() << "\nResult initial_matrix:\n"
                   << matrix << "\nExpected matrix:\n"
                   << get_3_process_8_devices_matrix() << std::endl;
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
                  { 1, dev_5 },
              } } },
            { 2,
              { {
                  { 2, dev_6 },
                  { 2, dev_7 },
              } } },
        };
        pg_comm->set_collect_cluster_colored_plain_graphs(control_cluster_topology);

        top.set_mock_colored_cluster_graphs_to_merge({
            //to_check
            { process_index, { { { 0, dev_0 }, { 0, dev_1 } }, { { 0, dev_2 }, { 0, dev_3 } } } },

            { 1, { { { 1, dev_4 }, { 1, dev_5 } } } },

            { 2, { { { 2, dev_6 }, { 2, dev_7 } } } },
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

              { 1, { { { 1, dev_4 }, { 1, dev_5 } } } },
              { 2,
                {
                    { { 2, dev_6 }, { 2, dev_7 } },
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
                                          get_3_process_8_devices_matrix()));
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
        set_control_indices<ccl_scaleout_proxy<ccl_gpu_comm>>(
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
        if (!res) {
            output << "\nLOG:\n" << ss.str() << std::endl;
            output << descr;
            UT_ASSERT(res, descr);
        }
    }
}

TEST_F(router_fixture, SCALEUP_NUMA_TEST_3_process_8_devices_process_1) {
    using namespace utils;
    using namespace native;
    using namespace native::detail;

    size_t process_index = 1;

    constexpr ccl::group_split_type topology = ccl::group_split_type::cluster;
    {
        process_creator_params params =
            prepare_process_params(process_index,
                                   { { 0, { dev_2, dev_3 } }, { 1, { dev_4, dev_5 } } },

                                   { { 0, { dev_0, dev_1 } },
                                     { process_index, { dev_2, dev_3, dev_4, dev_5 } },
                                     { 2, { dev_6, dev_7 } } });

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
                      get_3_process_8_devices_matrix()));
        //check matrix
        if (matrix != get_3_process_8_devices_matrix()) {
            output << ss.str() << "\nResult initial_matrix:\n"
                   << matrix << "\nExpected matrix:\n"
                   << get_3_process_8_devices_matrix() << std::endl;
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
                    { process_index, dev_2 },
                    { process_index, dev_3 },
                },
                {
                    { process_index, dev_4 },
                    { process_index, dev_5 },
                } } },
            { 2,
              { {
                  { 2, dev_6 },
                  { 2, dev_7 },
              } } },
        };
        pg_comm->set_collect_cluster_colored_plain_graphs(control_cluster_topology);

        top.set_mock_colored_cluster_graphs_to_merge({
            //to_check
            { 0, { { { 0, dev_0 }, { 0, dev_1 } } } },

            { process_index, //in terms of communicator our process_index is` zero`
              { { { process_index, dev_2 }, { process_index, dev_3 } },
                { { process_index, dev_4 }, { process_index, dev_5 } } } },
            { 2, { { { 2, dev_6 }, { 2, dev_7 } } } },
        });

        top.set_mock_merged_colored_cluster_graphs(
            { //to_check
              { process_index,
                { { { process_index, dev_2 }, { process_index, dev_3 } },
                  { { process_index, dev_4 }, { process_index, dev_5 } } } } },
            { //to_replace
              { 0,
                {
                    { { 0, dev_0 }, { 0, dev_1 } },
                } },

              { process_index,
                { { { process_index, dev_2 }, { process_index, dev_3 } },
                  { { process_index, dev_4 }, { process_index, dev_5 } } } },
              { 2,
                {
                    { { 2, dev_6 }, { 2, dev_7 } },
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
                                          get_3_process_8_devices_matrix()));
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
        set_control_indices<ccl_scaleout_proxy<ccl_gpu_comm>>(
            expected_thread_results, 0, optional_indices{ true, { 2 }, { 0 } });
        set_control_indices<ccl_numa_proxy<ccl_gpu_comm>>(
            expected_thread_results, 0, optional_indices{ true, { 3 }, { 1 } });
        set_control_indices<ccl_scaleout_proxy<ccl_gpu_comm>>(
            expected_thread_results, 1, optional_indices{ true, { 4 }, { 0 } });
        set_control_indices<ccl_numa_proxy<ccl_gpu_comm>>(
            expected_thread_results, 1, optional_indices{ true, { 5 }, { 1 } });
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

TEST_F(router_fixture, SCALEUP_NUMA_TEST_3_process_8_devices_process_2) {
    using namespace utils;
    using namespace native;
    using namespace native::detail;

    size_t process_index = 2;

    constexpr ccl::group_split_type topology = ccl::group_split_type::cluster;
    {
        process_creator_params params =
            prepare_process_params(process_index,
                                   { { 0, { dev_4, dev_5 } }, { 1, { dev_6, dev_7 } } },

                                   { { 0, { dev_0, dev_1 } },
                                     { 1, { dev_2, dev_3 } },
                                     { 2, { dev_4, dev_5, dev_6, dev_7 } } });

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
                      get_3_process_8_devices_matrix()));
        //check matrix
        if (matrix != get_3_process_8_devices_matrix()) {
            output << ss.str() << "\nResult initial_matrix:\n"
                   << matrix << "\nExpected matrix:\n"
                   << get_3_process_8_devices_matrix() << std::endl;
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
            { 1,
              { {
                  { 1, dev_2 },
                  { 1, dev_3 },
              } } },
            { process_index,
              { {
                    { process_index, dev_4 },
                    { process_index, dev_5 },
                },
                {
                    { process_index, dev_6 },
                    { process_index, dev_7 },
                } } },
        };
        pg_comm->set_collect_cluster_colored_plain_graphs(control_cluster_topology);

        top.set_mock_colored_cluster_graphs_to_merge({
            //to_check
            { 0, { { { 0, dev_0 }, { 0, dev_1 } } } },

            { 1, { { { 1, dev_2 }, { 1, dev_3 } } } },

            { process_index,
              { { { process_index, dev_4 }, { process_index, dev_5 } },
                { { process_index, dev_6 }, { process_index, dev_7 } } } },
        });

        top.set_mock_merged_colored_cluster_graphs(
            { //to_check
              { process_index,
                { { { process_index, dev_4 }, { process_index, dev_5 } },
                  { { process_index, dev_6 }, { process_index, dev_7 } } } } },
            { //to_replace
              { 0,
                {
                    { { 0, dev_0 }, { 0, dev_1 } },
                } },

              { 1,
                {
                    { { 1, dev_2 }, { 1, dev_3 } },
                } },
              { process_index,
                {
                    { { process_index, dev_4 }, { process_index, dev_5 } },
                    { { process_index, dev_6 }, { process_index, dev_7 } },
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
                                          get_3_process_8_devices_matrix()));
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
        set_control_indices<ccl_scaleout_proxy<ccl_gpu_comm>>(
            expected_thread_results, 0, optional_indices{ true, { 4 }, { 0 } });
        set_control_indices<ccl_numa_proxy<ccl_gpu_comm>>(
            expected_thread_results, 0, optional_indices{ true, { 5 }, { 1 } });
        set_control_indices<ccl_scaleout_proxy<ccl_gpu_comm>>(
            expected_thread_results, 1, optional_indices{ true, { 6 }, { 0 } });
        set_control_indices<ccl_numa_proxy<ccl_gpu_comm>>(
            expected_thread_results, 1, optional_indices{ true, { 7 }, { 1 } });
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

const native::detail::adjacency_matrix& get_3_process_6_devices_mix_matrix() {
    static native::detail::adjacency_matrix matrix{
        { dev_0,
          { { dev_0, 1 }, { dev_1, 0 }, { dev_2, 0 }, { dev_3, 0 }, { dev_4, 0 }, { dev_5, 0 } } },
        { dev_1,
          { { dev_0, 0 }, { dev_1, 1 }, { dev_2, 0 }, { dev_3, 0 }, { dev_4, 0 }, { dev_5, 0 } } },
        { dev_2,
          { { dev_0, 0 }, { dev_1, 0 }, { dev_2, 1 }, { dev_3, 0 }, { dev_4, 0 }, { dev_5, 0 } } },
        { dev_3,
          { { dev_0, 0 }, { dev_1, 0 }, { dev_2, 0 }, { dev_3, 1 }, { dev_4, 0 }, { dev_5, 0 } } },
        { dev_4,
          { { dev_0, 0 }, { dev_1, 0 }, { dev_2, 0 }, { dev_3, 0 }, { dev_4, 1 }, { dev_5, 0 } } },
        { dev_5,
          { { dev_0, 0 }, { dev_1, 0 }, { dev_2, 0 }, { dev_3, 0 }, { dev_4, 0 }, { dev_5, 1 } } },
    };
    return matrix;
}

TEST_F(router_fixture, SCALEUP_NUMA_TEST_3_process_6_devices_mix_process_0) {
    using namespace utils;
    using namespace native;
    using namespace native::detail;

    size_t process_index = 0;

    constexpr ccl::group_split_type topology = ccl::group_split_type::cluster;
    {
        process_creator_params params =
            prepare_process_params(process_index,
                                   { { 0,
                                       {
                                           dev_0,
                                       } },
                                     { 1,
                                       {
                                           dev_1,
                                       } } },

                                   { { process_index, { dev_0, dev_1 } },
                                     { 1, { dev_2, dev_3 } },
                                     { 2, { dev_4, dev_5 } } });

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
                      get_3_process_6_devices_mix_matrix()));
        //check matrix
        if (matrix != get_3_process_6_devices_mix_matrix()) {
            output << ss.str() << "\nResult initial_matrix:\n"
                   << matrix << "\nExpected matrix:\n"
                   << get_3_process_6_devices_mix_matrix() << std::endl;
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
            { process_index,
              { {
                    { process_index, dev_0 },
                },
                {
                    { process_index, dev_1 },
                } } },
            { 1,
              { {
                    { 1, dev_2 },
                },
                {
                    { 1, dev_3 },
                } } },
            { 2,
              { {
                    { 2, dev_4 },
                },
                {
                    { 2, dev_5 },
                } } },
        };
        pg_comm->set_collect_cluster_colored_plain_graphs(control_cluster_topology);

        top.set_mock_colored_cluster_graphs_to_merge({
            //to_check
            { process_index, { { { process_index, dev_0 } }, { { process_index, dev_1 } } } },

            { 1, { { { 1, dev_2 } }, { { 1, dev_3 } } } },

            { 2, { { { 2, dev_4 } }, { { 2, dev_5 } } } },
        });

        top.set_mock_merged_colored_cluster_graphs(
            { //to_check
              { process_index, { { { process_index, dev_0 } }, { { process_index, dev_1 } } } } },
            { //to_replace
              { process_index, { { { process_index, dev_0 } }, { { process_index, dev_1 } } } },

              { 1, { { { 1, dev_2 } }, { { 1, dev_3 } } } },
              { 2,
                {
                    { { 2, dev_4 } },
                    { { 2, dev_5 } },
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
                                          get_3_process_6_devices_mix_matrix()));
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
        set_control_indices<ccl_scaleout_proxy<ccl_numa_proxy<ccl_gpu_comm>>>(
            expected_thread_results, 0, optional_indices{ true, { 0 }, { 0 } });
        set_control_indices<ccl_scaleout_proxy<ccl_numa_proxy<ccl_gpu_comm>>>(
            expected_thread_results, 1, optional_indices{ true, { 1 }, { 0 } });
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
} // namespace processgroup_topology_suite
