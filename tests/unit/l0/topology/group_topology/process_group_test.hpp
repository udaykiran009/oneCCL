#pragma once
#include <initializer_list>
#include "../topology_utils.hpp"

namespace topology_suite {
struct allied_process_group_ring_topology_mock : public native::allied_process_group_ring_topology {
    allied_process_group_ring_topology_mock(size_t process_idx,
                                            size_t process_num,
                                            native::process_group_context& ctx,
                                            native::device_storage& devs,
                                            size_t cluster_rank_offset,
                                            size_t cluster_size)
            : allied_process_group_ring_topology(process_idx,
                                                 process_num,
                                                 ctx,
                                                 devs,
                                                 cluster_rank_offset,
                                                 cluster_size) {}

    native::detail::global_plain_graphs merge_allied_nodes_plain_graphs(
        std::ostream& out,
        const ccl::cluster_device_indices_type& cluster_indices,
        size_t process_index,
        const native::detail::global_sorted_plain_graphs& cluster_graphs,
        native::detail::p2p_rating_function ping) override {
        out << "<UNIT_TEST mock:" << __PRETTY_FUNCTION__ << ">" << std::endl;

        if (!overrided_cluster_graphs_to_replace.empty()) {
            for (const auto& initial : cluster_graphs) {
                auto tested_it = overrided_cluster_graphs_to_check.find(initial.first);
                if (tested_it == overrided_cluster_graphs_to_check.end()) {
                    throw std::runtime_error(
                        std::string("failed: mock overrided_cluster_graphs_to_merge") +
                        " - doesn't contains process: " + std::to_string(initial.first));
                }

                if (tested_it->second != initial.second) {
                    throw std::runtime_error(
                        std::string("failed: mock overrided_cluster_graphs_to_merge") +
                        " - contains unexpected graphs for process: " +
                        std::to_string(initial.first) + "\nExpected:\n" +
                        native::detail::to_string(overrided_cluster_graphs_to_check) + "\n,Got:\n" +
                        native::detail::to_string(cluster_graphs));
                }
            }
            return allied_process_group_ring_topology::merge_allied_nodes_plain_graphs(
                out, cluster_indices, process_index, overrided_cluster_graphs_to_replace, ping);
        }

        return allied_process_group_ring_topology::merge_allied_nodes_plain_graphs(
            out, cluster_indices, process_index, cluster_graphs, ping);
    }

    native::detail::global_colored_plain_graphs merge_allied_nodes_in_colored_plain_graphs(
        std::ostream& out,
        const ccl::cluster_device_indices_type& cluster_indices,
        size_t process_index,
        size_t process_count,
        const native::detail::global_sorted_colored_plain_graphs& cluster_graphs,
        native::detail::p2p_rating_function ping) override {
        out << "<UNIT_TEST mock:" << __PRETTY_FUNCTION__ << ">" << std::endl;

        if (!overrided_colored_cluster_graphs_to_replace.empty()) {
            for (const auto& initial : cluster_graphs) {
                auto tested_it = overrided_colored_cluster_graphs_to_check.find(initial.first);
                if (tested_it == overrided_colored_cluster_graphs_to_check.end()) {
                    throw std::runtime_error(
                        std::string("failed: mock overrided_colored_cluster_graphs_to_merge") +
                        " - doesn't contains process: " + std::to_string(initial.first));
                }

                if (tested_it->second != initial.second) {
                    throw std::runtime_error(
                        std::string("failed: mock overrided_colored_cluster_graphs_to_merge") +
                        " - contains unexpected graphs for process: " +
                        std::to_string(initial.first) + "\nExpected:\n" +
                        native::detail::to_string(overrided_colored_cluster_graphs_to_check) +
                        "\n,Got:\n" + native::detail::to_string(cluster_graphs));
                }
            }
            return allied_process_group_ring_topology::merge_allied_nodes_in_colored_plain_graphs(
                out,
                cluster_indices,
                process_index,
                process_count,
                overrided_colored_cluster_graphs_to_replace,
                ping);
        }

        return allied_process_group_ring_topology::merge_allied_nodes_in_colored_plain_graphs(
            out, cluster_indices, process_index, process_count, cluster_graphs, ping);
    }

    ccl::process_device_indices_type create_scaleout_devices_in_graphs_for_process(
        size_t process_index,
        size_t cluster_size,
        native::detail::global_sorted_plain_graphs& cluster_graphs,
        std::ostream& out) override {
        out << "<UNIT_TEST mock:" << __PRETTY_FUNCTION__ << ">" << std::endl;
        size_t mock_cluster_size = cluster_size;
        if (!overrided_merged_cluster_graphs_to_check.empty()) {
            for (const auto& initial : cluster_graphs) {
                auto tested_it = overrided_merged_cluster_graphs_to_check.find(initial.first);
                if (tested_it == overrided_merged_cluster_graphs_to_check.end()) {
                    throw std::runtime_error(
                        std::string("failed: mock overrided_merged_cluster_graphs_to_check") +
                        " - doesn't contains process: " + std::to_string(initial.first));
                }

                if (tested_it->second != initial.second) {
                    throw std::runtime_error(
                        std::string("failed: mock overrided_merged_cluster_graphs_to_check") +
                        " - contains unexpected graphs for process: " +
                        std::to_string(initial.first) + "\nExpected:\n" +
                        native::detail::to_string(overrided_merged_cluster_graphs_to_check) +
                        "\n,Got:\n" + native::detail::to_string(cluster_graphs));
                }
            }
            cluster_graphs = overrided_merged_cluster_graphs_to_replace;
            mock_cluster_size = cluster_graphs.size();
        }
        return allied_process_group_ring_topology::create_scaleout_devices_in_graphs_for_process(
            process_index, mock_cluster_size, cluster_graphs, out);
    }

    ccl::process_device_indices_type create_scaleout_devices_in_colored_graphs_for_process(
        size_t process_index,
        size_t cluster_size,
        native::detail::global_sorted_colored_plain_graphs& cluster_graphs,
        native::detail::global_sorted_colored_plain_graphs& initial_cluster_graphs,
        std::ostream& out) override {
        out << "<UNIT_TEST mock:" << __PRETTY_FUNCTION__ << ">" << std::endl;
        size_t mock_cluster_size = cluster_size;
        if (!overrided_colored_merged_cluster_graphs_to_check.empty()) {
            for (const auto& initial : cluster_graphs) {
                auto tested_it =
                    overrided_colored_merged_cluster_graphs_to_check.find(initial.first);
                if (tested_it == overrided_colored_merged_cluster_graphs_to_check.end()) {
                    throw std::runtime_error(
                        std::string(
                            "failed: mock overrided_colored_merged_cluster_graphs_to_check") +
                        " - doesn't contains process: " + std::to_string(initial.first));
                }

                if (tested_it->second != initial.second) {
                    throw std::runtime_error(
                        std::string(
                            "failed: mock overrided_colored_merged_cluster_graphs_to_check") +
                        " - contains unexpected graphs for process: " +
                        std::to_string(initial.first) + "\nExpected:\n" +
                        native::detail::to_string(
                            overrided_colored_merged_cluster_graphs_to_check) +
                        "\n,Got:\n" + native::detail::to_string(cluster_graphs));
                }
            }
            cluster_graphs = overrided_colored_merged_cluster_graphs_to_replace;
            mock_cluster_size = cluster_graphs.size();
        }
        return allied_process_group_ring_topology::
            create_scaleout_devices_in_colored_graphs_for_process(
                process_index, mock_cluster_size, cluster_graphs, initial_cluster_graphs, out);
    }

    void set_mock_cluster_graphs_to_merge(
        std::initializer_list<typename native::detail::global_sorted_plain_graphs::value_type>
            to_check,
        std::initializer_list<typename native::detail::global_sorted_plain_graphs::value_type>
            to_replace = {}) {
        overrided_cluster_graphs_to_check = to_check;
        overrided_cluster_graphs_to_replace = to_replace;
        if (overrided_cluster_graphs_to_replace.empty()) {
            overrided_cluster_graphs_to_replace = overrided_cluster_graphs_to_check;
        }
    }

    void set_mock_colored_cluster_graphs_to_merge(
        std::initializer_list<
            typename native::detail::global_sorted_colored_plain_graphs::value_type> to_check,
        std::initializer_list<
            typename native::detail::global_sorted_colored_plain_graphs::value_type>
            to_replace = {}) {
        overrided_colored_cluster_graphs_to_check = to_check;
        overrided_colored_cluster_graphs_to_replace = to_replace;
        if (overrided_colored_cluster_graphs_to_replace.empty()) {
            overrided_colored_cluster_graphs_to_replace = overrided_colored_cluster_graphs_to_check;
        }
    }

    void set_mock_merged_cluster_graphs(
        std::initializer_list<typename native::detail::global_sorted_plain_graphs::value_type>
            to_check,
        std::initializer_list<typename native::detail::global_sorted_plain_graphs::value_type>
            to_replace = {}) {
        overrided_merged_cluster_graphs_to_check = to_check;
        overrided_merged_cluster_graphs_to_replace = to_replace;
        if (overrided_merged_cluster_graphs_to_replace.empty()) {
            overrided_merged_cluster_graphs_to_replace = overrided_merged_cluster_graphs_to_check;
        }
    }

    void set_mock_merged_colored_cluster_graphs(
        std::initializer_list<
            typename native::detail::global_sorted_colored_plain_graphs::value_type> to_check,
        std::initializer_list<
            typename native::detail::global_sorted_colored_plain_graphs::value_type>
            to_replace = {}) {
        overrided_colored_merged_cluster_graphs_to_check = to_check;
        overrided_colored_merged_cluster_graphs_to_replace = to_replace;
        if (overrided_colored_merged_cluster_graphs_to_replace.empty()) {
            overrided_colored_merged_cluster_graphs_to_replace =
                overrided_colored_merged_cluster_graphs_to_check;
        }
    }

private:
    native::detail::global_sorted_plain_graphs overrided_cluster_graphs_to_check;
    native::detail::global_sorted_plain_graphs overrided_cluster_graphs_to_replace;
    native::detail::global_sorted_colored_plain_graphs overrided_colored_cluster_graphs_to_check;
    native::detail::global_sorted_colored_plain_graphs overrided_colored_cluster_graphs_to_replace;

    native::detail::global_sorted_plain_graphs overrided_merged_cluster_graphs_to_check;
    native::detail::global_sorted_plain_graphs overrided_merged_cluster_graphs_to_replace;
    native::detail::global_sorted_colored_plain_graphs
        overrided_colored_merged_cluster_graphs_to_check;
    native::detail::global_sorted_colored_plain_graphs
        overrided_colored_merged_cluster_graphs_to_replace;
};

TEST_F(router_fixture, single_process_group_single_threads_single_device_topology_test) {
    using namespace utils;
    using namespace native;

    size_t process_index = 0;
    constexpr ccl::group_split_type topology = ccl::group_split_type::cluster;
    {
        //emulate last thread barrier creation
        //prepare thread context
        process_creator_params params = prepare_process_params(
            process_index,
            { { 0,
                { ccl::device_index_type(0, 0, ccl::unused_index_value),
                  ccl::device_index_type(0, 0, ccl::unused_index_value),
                  ccl::device_index_type(0, 0, ccl::unused_index_value) } } },
            { { process_index,
                { ccl::device_index_type(0, 0, ccl::unused_index_value),
                  ccl::device_index_type(0, 0, ccl::unused_index_value),
                  ccl::device_index_type(0, 0, ccl::unused_index_value) } } });
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
            allied_process_group_ring_topology::build_p2p_capability_matrix(ss,
                                                                            params.total_node_mask);

        std::vector<ccl::device_indices_type> ipc_device_indices =
            process_group_context::get_ipc_device_indices_for_id(process_index,
                                                                 params.total_node_mask);
        UT_ASSERT(ipc_device_indices.empty(),
                  "ipc_device_indices should be empty for single node case");

        //test topology creation
        try {
            res = top.build_all(ss, tg_comm->per_thread_indices, ipc_device_indices, matrix);
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
        std::tie(res, descr) = check_ring_multiple_topologies<topology, process_group_context>(
            pg_comm->process_device_topology, params.thread_ids, expected_thread_results);
        UT_ASSERT(res, descr);
    }
}

TEST_F(router_fixture, ally_process_group_topology_test) {
    using namespace utils;
    using namespace native;

    size_t process_index = 0;
    constexpr ccl::group_split_type topology = ccl::group_split_type::cluster;
    {
        //emulate last thread barrier creation
        process_creator_params params = prepare_process_params(
            process_index,
            { { 0, { ccl::device_index_type(0, 0, ccl::unused_index_value) } },
              { 1, { ccl::device_index_type(0, 0, ccl::unused_index_value) } } },
            { { process_index,
                { ccl::device_index_type(0, 0, ccl::unused_index_value),
                  ccl::device_index_type(0, 0, ccl::unused_index_value) } } });

        allied_process_group_ring_topology top(process_index,
                                               params.process_ids.size(),
                                               *pg_comm,
                                               *pg_comm->gpu_device_storage,
                                               params.cluster_device_rank_offset,
                                               params.cluster_device_size);

        std::stringstream ss;
        native::detail::adjacency_matrix matrix =
            allied_process_group_ring_topology::build_p2p_capability_matrix(ss,
                                                                            params.total_node_mask);
        output << ss.str() << "\nResult matrix:\n" << matrix << std::endl;
        ss.clear();

        std::vector<ccl::device_indices_type> ipc_device_indices =
            process_group_context::get_ipc_device_indices_for_id(process_index,
                                                                 params.total_node_mask);
        UT_ASSERT(ipc_device_indices.empty(),
                  "ipc_device_indices should be empty for single node case");

        bool res;
        std::string descr;

        res = top.build_all(ss, tg_comm->per_thread_indices, ipc_device_indices, matrix);
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

        std::tie(res, descr) = check_ring_multiple_topologies<topology, process_group_context>(
            pg_comm->process_device_topology, params.thread_ids, expected_thread_results);
        UT_ASSERT(res, descr);
    }

    {
        //emulate last thread barrier creation
        process_creator_params params = prepare_process_params(
            process_index,
            { { 0,
                { ccl::device_index_type(0, 0, ccl::unused_index_value),
                  ccl::device_index_type(0, 0, ccl::unused_index_value),
                  ccl::device_index_type(0, 0, ccl::unused_index_value) } },
              { 1,
                { ccl::device_index_type(0, 1, ccl::unused_index_value),
                  ccl::device_index_type(0, 1, ccl::unused_index_value),
                  ccl::device_index_type(0, 2, ccl::unused_index_value) } } },
            { { process_index,
                { ccl::device_index_type(0, 0, ccl::unused_index_value),
                  ccl::device_index_type(0, 0, ccl::unused_index_value),
                  ccl::device_index_type(0, 0, ccl::unused_index_value),
                  ccl::device_index_type(0, 1, ccl::unused_index_value),
                  ccl::device_index_type(0, 1, ccl::unused_index_value),
                  ccl::device_index_type(0, 2, ccl::unused_index_value) } },
              { 1, { ccl::device_index_type(0, 3, ccl::unused_index_value) } } });

        allied_process_group_ring_topology_mock top(process_index,
                                                    params.process_ids.size(),
                                                    *pg_comm,
                                                    *pg_comm->gpu_device_storage,
                                                    params.cluster_device_rank_offset,
                                                    params.cluster_device_size);

        std::stringstream ss;
        native::detail::adjacency_matrix matrix =
            allied_process_group_ring_topology::build_p2p_capability_matrix(ss,
                                                                            params.total_node_mask);
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
        top.set_mock_colored_cluster_graphs_to_merge(
            { { process_index,
                { { { process_index, ccl::device_index_type(0, 0, ccl::unused_index_value) },
                    { process_index, ccl::device_index_type(0, 0, ccl::unused_index_value) },
                    { process_index, ccl::device_index_type(0, 0, ccl::unused_index_value) },
                    { process_index, ccl::device_index_type(0, 1, ccl::unused_index_value) },
                    { process_index, ccl::device_index_type(0, 1, ccl::unused_index_value) },
                    { process_index, ccl::device_index_type(0, 2, ccl::unused_index_value) } } } },
              { 1, { { { 1, ccl::device_index_type(0, 3, ccl::unused_index_value) } } } } });
        top.set_mock_merged_colored_cluster_graphs(
            { { process_index,
                { { { process_index, ccl::device_index_type(0, 0, ccl::unused_index_value) },
                    { process_index, ccl::device_index_type(0, 0, ccl::unused_index_value) },
                    { process_index, ccl::device_index_type(0, 0, ccl::unused_index_value) },
                    { process_index, ccl::device_index_type(0, 1, ccl::unused_index_value) },
                    { process_index, ccl::device_index_type(0, 1, ccl::unused_index_value) },
                    { process_index, ccl::device_index_type(0, 2, ccl::unused_index_value) },
                    { 1, ccl::device_index_type(0, 3, ccl::unused_index_value) } } } },
              { 1,
                { {
                    { 1, ccl::device_index_type(0, 3, ccl::unused_index_value) },
                    { process_index, ccl::device_index_type(0, 0, ccl::unused_index_value) },
                    { process_index, ccl::device_index_type(0, 0, ccl::unused_index_value) },
                    { process_index, ccl::device_index_type(0, 0, ccl::unused_index_value) },
                    { process_index, ccl::device_index_type(0, 1, ccl::unused_index_value) },
                    { process_index, ccl::device_index_type(0, 1, ccl::unused_index_value) },
                    { process_index, ccl::device_index_type(0, 2, ccl::unused_index_value) },
                } } } });
        try {
            res = top.build_all(ss, tg_comm->per_thread_indices, ipc_device_indices, matrix);
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

        std::tie(res, descr) = check_ring_multiple_topologies<topology, process_group_context>(
            pg_comm->process_device_topology, params.thread_ids, expected_thread_results);
        UT_ASSERT(res, descr);
    }

    {
        //emulate last thread barrier creation
        process_creator_params params = prepare_process_params(
            process_index,
            { { 0,
                { ccl::device_index_type(0, 0, ccl::unused_index_value),
                  ccl::device_index_type(0, 2, ccl::unused_index_value),
                  ccl::device_index_type(0, 0, ccl::unused_index_value) } },
              { 1,
                { ccl::device_index_type(0, 1, ccl::unused_index_value),
                  ccl::device_index_type(0, 2, ccl::unused_index_value),
                  ccl::device_index_type(0, 0, ccl::unused_index_value) } } },
            { { 0,
                { ccl::device_index_type(0, 0, ccl::unused_index_value),
                  ccl::device_index_type(0, 2, ccl::unused_index_value),
                  ccl::device_index_type(0, 0, ccl::unused_index_value),
                  ccl::device_index_type(0, 1, ccl::unused_index_value),
                  ccl::device_index_type(0, 2, ccl::unused_index_value),
                  ccl::device_index_type(0, 0, ccl::unused_index_value) } },
              { 1,
                { ccl::device_index_type(0, 3, ccl::unused_index_value),
                  ccl::device_index_type(0, 1, ccl::unused_index_value),
                  ccl::device_index_type(0, 2, ccl::unused_index_value) } } });

        allied_process_group_ring_topology_mock top(process_index,
                                                    params.process_ids.size(),
                                                    *pg_comm,
                                                    *pg_comm->gpu_device_storage,
                                                    params.cluster_device_rank_offset,
                                                    params.cluster_device_size);

        std::stringstream ss;
        native::detail::adjacency_matrix matrix =
            allied_process_group_ring_topology::build_p2p_capability_matrix(ss,
                                                                            params.total_node_mask);
        output << ss.str() << "\nResult matrix:\n" << matrix << std::endl;
        ss.clear();

        std::vector<ccl::device_indices_type> ipc_device_indices =
            process_group_context::get_ipc_device_indices_for_id(process_index,
                                                                 params.total_node_mask);
        UT_ASSERT(ipc_device_indices.size() == 1, "three ipc_device_indices should be");
        UT_ASSERT(ipc_device_indices[0].size() == 3, "three ipc_device_indices should be");

        bool res;
        std::string descr;

        top.set_mock_colored_cluster_graphs_to_merge(
            { { process_index,
                { { { process_index, ccl::device_index_type(0, 0, ccl::unused_index_value) },
                    { process_index, ccl::device_index_type(0, 0, ccl::unused_index_value) },
                    { process_index, ccl::device_index_type(0, 2, ccl::unused_index_value) },
                    { process_index, ccl::device_index_type(0, 0, ccl::unused_index_value) },
                    { process_index, ccl::device_index_type(0, 1, ccl::unused_index_value) },
                    { process_index, ccl::device_index_type(0, 2, ccl::unused_index_value) } } } },
              { 1,
                { { { 1, ccl::device_index_type(0, 1, ccl::unused_index_value) },
                    { 1, ccl::device_index_type(0, 2, ccl::unused_index_value) },
                    { 1, ccl::device_index_type(0, 3, ccl::unused_index_value) } } } } });
        try {
            res = top.build_all(ss, tg_comm->per_thread_indices, ipc_device_indices, matrix);
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
            optional_indices{ true, { 2 }, {} },
            optional_indices{ true, { 1 }, {} },
            optional_indices{ false, {}, {} },
            optional_indices{ true, { 3 }, {} },
            optional_indices{ true, { 0 }, {} },
            optional_indices{ true, {}, {} },
        }));
        //thread 1
        expected_thread_results.push_back(
            expected_indices_tuple({ optional_indices{ true, { 4 }, {} },
                                     optional_indices{ true, { 3, 5 }, {} },
                                     optional_indices{ false, {}, {} },
                                     optional_indices{ false, {}, {} },
                                     optional_indices{ false, {}, {} },
                                     optional_indices{ false, {}, {} },
                                     optional_indices{ true, { 6 }, {} } }));

        std::tie(res, descr) = check_ring_multiple_topologies<topology, process_group_context>(
            pg_comm->process_device_topology, params.thread_ids, expected_thread_results);
        UT_ASSERT(res, descr);
    }
}

TEST_F(router_fixture, inter_process_scale_up_process_group_topology_test) {
    using namespace utils;
    using namespace native;
    using namespace native::detail;

    size_t process_index = 0;

    constexpr ccl::group_split_type topology = ccl::group_split_type::process_group_torn_apart_ring;
    {
        output << "TEST: scaleup between thread groups in one process group" << std::endl;
        process_creator_params params = prepare_process_params(
            process_index,
            { { 0,
                { ccl::device_index_type(0, 0, ccl::unused_index_value),
                  ccl::device_index_type(0, 1, ccl::unused_index_value) } },
              { 1,
                { ccl::device_index_type(0, 2, ccl::unused_index_value),
                  ccl::device_index_type(0, 3, ccl::unused_index_value) } } },
            { { process_index,
                { ccl::device_index_type(0, 0, ccl::unused_index_value),
                  ccl::device_index_type(0, 1, ccl::unused_index_value),
                  ccl::device_index_type(0, 2, ccl::unused_index_value),
                  ccl::device_index_type(0, 3, ccl::unused_index_value) } } });

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

        allied_process_group_ring_topology_mock top(process_index,
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
        UT_ASSERT(ipc_device_indices.empty(),
                  "ipc_device_indices should be empty for single node case");

        bool res;
        std::string descr;

        top.set_mock_colored_cluster_graphs_to_merge({
            { process_index,
              { { { process_index, ccl::device_index_type(0, 0, ccl::unused_index_value) },
                  { process_index, ccl::device_index_type(0, 1, ccl::unused_index_value) } },
                { { process_index, ccl::device_index_type(0, 2, ccl::unused_index_value) },
                  { process_index, ccl::device_index_type(0, 3, ccl::unused_index_value) } } } },
        });

        try {
            res = top.build_all(ss,
                                tg_comm->per_thread_indices,
                                ipc_device_indices,
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
        std::vector<expected_indices_tuple> expected_thread_results(
            { expected_indices_tuple({ optional_indices{ true, { 0 }, { 0 } },
                                       optional_indices{ false, {}, {} },
                                       optional_indices{ false, { 2 }, { 2 } },
                                       optional_indices{ false, {}, {} },
                                       optional_indices{ false, {}, {} },
                                       optional_indices{ false, {}, {} },
                                       optional_indices{ false, {}, {} },
                                       optional_indices{ true, { 1 }, { 1 } },
                                       optional_indices{ false, {}, {} } }),
              expected_indices_tuple({ optional_indices{ true, { 2 }, { 0 } },
                                       optional_indices{ false, {}, {} },
                                       optional_indices{ false, { 0 }, { 0 } },
                                       optional_indices{ false, {}, {} },
                                       optional_indices{ false, {}, {} },
                                       optional_indices{ false, {}, {} },
                                       optional_indices{ false, {}, {} },
                                       optional_indices{ true, { 3 }, { 1 } },
                                       optional_indices{ false, {}, {} } }) });

        std::tie(res, descr) = check_ring_multiple_topologies<topology, process_group_context>(
            pg_comm->process_device_topology, params.thread_ids, expected_thread_results);
        if (!res) {
            output << "\nLOG:\n" << ss.str() << std::endl;
            output << descr;
            UT_ASSERT(res, descr);
        }
    }
}

TEST_F(router_fixture, several_processes_with_inner_scale_up_in_process_group_topology_test) {
    using namespace utils;
    using namespace native;
    using namespace native::detail;

    size_t process_index = 1;

    constexpr ccl::group_split_type topology = ccl::group_split_type::process_group_torn_apart_ring;
    {
        output << "TEST: scaleup between thread groups in one process group" << std::endl;

        process_creator_params params = prepare_process_params(
            process_index,
            { { 0,
                { ccl::device_index_type(0, 0, ccl::unused_index_value),
                  ccl::device_index_type(0, 1, ccl::unused_index_value) } },
              { 1,
                { ccl::device_index_type(0, 2, ccl::unused_index_value),
                  ccl::device_index_type(0, 3, ccl::unused_index_value) } } },
            { { 0,
                { ccl::device_index_type(0, 0, ccl::unused_index_value),
                  ccl::device_index_type(0, 1, ccl::unused_index_value) } },
              { process_index,
                { ccl::device_index_type(0, 0, ccl::unused_index_value),
                  ccl::device_index_type(0, 1, ccl::unused_index_value),
                  ccl::device_index_type(0, 2, ccl::unused_index_value),
                  ccl::device_index_type(0, 3, ccl::unused_index_value) } },
              { 2,
                { ccl::device_index_type(0, 2, ccl::unused_index_value),
                  ccl::device_index_type(0, 3, ccl::unused_index_value) } } });

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

        allied_process_group_ring_topology_mock top(process_index,
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

        top.set_mock_colored_cluster_graphs_to_merge(
            {
                //to_check
                { 0, //in terms of communicator our process_index is` zero`
                  { { { 0, ccl::device_index_type(0, 0, ccl::unused_index_value) },
                      { 0, ccl::device_index_type(0, 1, ccl::unused_index_value) } },
                    { { 0, ccl::device_index_type(0, 2, ccl::unused_index_value) },
                      { 0, ccl::device_index_type(0, 3, ccl::unused_index_value) } } } },
            },
            {
                //to_replace
                { 0,
                  {
                      { { 0, ccl::device_index_type(0, 0, ccl::unused_index_value) },
                        { 0, ccl::device_index_type(0, 1, ccl::unused_index_value) } },
                  } },
                { process_index,
                  { { { 1, ccl::device_index_type(0, 0, ccl::unused_index_value) },
                      { 1, ccl::device_index_type(0, 1, ccl::unused_index_value) } },
                    { { 1, ccl::device_index_type(0, 2, ccl::unused_index_value) },
                      { 1, ccl::device_index_type(0, 3, ccl::unused_index_value) } } } },
                { 2,
                  { { { 2, ccl::device_index_type(0, 2, ccl::unused_index_value) },
                      { 2, ccl::device_index_type(0, 3, ccl::unused_index_value) } } } },
            });

        top.set_mock_merged_colored_cluster_graphs(
            { //to_check
              { 0, //process_index is zero, because communicator
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
        try {
            res = top.build_all(ss,
                                tg_comm->per_thread_indices,
                                ipc_device_indices,
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
        std::vector<expected_indices_tuple> expected_thread_results(
            { expected_indices_tuple({ optional_indices{ true, { 0 }, { 0 } },
                                       optional_indices{ false, {}, {} },
                                       optional_indices{ false, { 2 }, { 2 } },
                                       optional_indices{ false, {}, {} },
                                       optional_indices{ false, {}, {} },
                                       optional_indices{ false, {}, {} },
                                       optional_indices{ false, {}, {} },
                                       optional_indices{ true, { 1 }, { 1 } },
                                       optional_indices{ false, {}, {} } }),
              expected_indices_tuple({ optional_indices{ true, { 2 }, { 0 } },
                                       optional_indices{ false, {}, {} },
                                       optional_indices{ false, { 0 }, { 0 } },
                                       optional_indices{ false, {}, {} },
                                       optional_indices{ false, {}, {} },
                                       optional_indices{ false, {}, {} },
                                       optional_indices{ false, {}, {} },
                                       optional_indices{ true, { 3 }, { 1 } },
                                       optional_indices{ false, {}, {} } }) });

        std::tie(res, descr) = check_ring_multiple_topologies<topology, process_group_context>(
            pg_comm->process_device_topology, params.thread_ids, expected_thread_results);
        if (!res) {
            output << "\nLOG:\n" << ss.str() << std::endl;
            output << descr;
            UT_ASSERT(res, descr);
        }
    }
}

TEST_F(router_fixture, scale_up_scale_out_process_group_topology_test) {
    using namespace utils;
    using namespace native;
    using namespace native::detail;

    size_t process_index = 0;

    constexpr ccl::group_split_type topology = ccl::group_split_type::process_group_torn_apart_ring;
    {
        output << "TEST: scaleup between thread groups in one process group" << std::endl;

        process_creator_params params = prepare_process_params(
            process_index,
            { { 0,
                { ccl::device_index_type(0, 0, ccl::unused_index_value),
                  ccl::device_index_type(0, 1, ccl::unused_index_value) } },
              { 1,
                { ccl::device_index_type(0, 2, ccl::unused_index_value),
                  ccl::device_index_type(0, 3, ccl::unused_index_value) } } },
            { { process_index,
                { ccl::device_index_type(0, 0, ccl::unused_index_value),
                  ccl::device_index_type(0, 1, ccl::unused_index_value),
                  ccl::device_index_type(0, 2, ccl::unused_index_value),
                  ccl::device_index_type(0, 3, ccl::unused_index_value) } },
              { 1, { ccl::device_index_type(0, 4, ccl::unused_index_value) } } });

        allied_process_group_ring_topology_mock top(process_index,
                                                    params.process_ids.size(),
                                                    *pg_comm,
                                                    *pg_comm->gpu_device_storage,
                                                    params.cluster_device_rank_offset,
                                                    params.cluster_device_size);
        adjacency_matrix expected_matrix{
            { ccl::device_index_type(0, 0, ccl::unused_index_value),
              { { ccl::device_index_type(0, 0, ccl::unused_index_value), 1 },
                { ccl::device_index_type(0, 1, ccl::unused_index_value), 1 },
                { ccl::device_index_type(0, 2, ccl::unused_index_value), 0 },
                { ccl::device_index_type(0, 3, ccl::unused_index_value), 0 },

                { ccl::device_index_type(0, 4, ccl::unused_index_value), 0 }

              } },
            { ccl::device_index_type(0, 1, ccl::unused_index_value),
              { { ccl::device_index_type(0, 0, ccl::unused_index_value), 1 },
                { ccl::device_index_type(0, 1, ccl::unused_index_value), 1 },
                { ccl::device_index_type(0, 2, ccl::unused_index_value), 0 },
                { ccl::device_index_type(0, 3, ccl::unused_index_value), 0 },

                { ccl::device_index_type(0, 4, ccl::unused_index_value), 0 } } },
            { ccl::device_index_type(0, 2, ccl::unused_index_value),
              { { ccl::device_index_type(0, 0, ccl::unused_index_value), 0 },
                { ccl::device_index_type(0, 1, ccl::unused_index_value), 0 },
                { ccl::device_index_type(0, 2, ccl::unused_index_value), 1 },
                { ccl::device_index_type(0, 3, ccl::unused_index_value), 1 },

                { ccl::device_index_type(0, 4, ccl::unused_index_value), 0 } } },
            { ccl::device_index_type(0, 3, ccl::unused_index_value),
              { { ccl::device_index_type(0, 0, ccl::unused_index_value), 0 },
                { ccl::device_index_type(0, 1, ccl::unused_index_value), 0 },
                { ccl::device_index_type(0, 2, ccl::unused_index_value), 1 },
                { ccl::device_index_type(0, 3, ccl::unused_index_value), 1 },

                { ccl::device_index_type(0, 4, ccl::unused_index_value), 0 } } },
            { ccl::device_index_type(0, 4, ccl::unused_index_value),
              { { ccl::device_index_type(0, 0, ccl::unused_index_value), 0 },
                { ccl::device_index_type(0, 1, ccl::unused_index_value), 0 },
                { ccl::device_index_type(0, 2, ccl::unused_index_value), 0 },
                { ccl::device_index_type(0, 3, ccl::unused_index_value), 0 },

                { ccl::device_index_type(0, 4, ccl::unused_index_value), 1 } } }
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

        top.set_mock_cluster_graphs_to_merge(
            { { process_index,
                { { ccl::device_index_type(0, 0, ccl::unused_index_value),
                    ccl::device_index_type(0, 1, ccl::unused_index_value) },
                  { ccl::device_index_type(0, 2, ccl::unused_index_value),
                    ccl::device_index_type(0, 3, ccl::unused_index_value) } } },
              { 1, { { ccl::device_index_type(0, 4, ccl::unused_index_value) } } } });

        top.set_mock_merged_cluster_graphs(
            { { process_index,
                {
                    { ccl::device_index_type(0, 0, ccl::unused_index_value),
                      ccl::device_index_type(0, 1, ccl::unused_index_value) },
                    { ccl::device_index_type(0, 2, ccl::unused_index_value),
                      ccl::device_index_type(0, 3, ccl::unused_index_value) },
                } },
              { 1,
                {
                    { ccl::device_index_type(0, 4, ccl::unused_index_value) },

                } } });

        try {
            res = top.build_all(ss,
                                tg_comm->per_thread_indices,
                                ipc_device_indices,
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
        std::vector<expected_indices_tuple> expected_thread_results(
            { expected_indices_tuple({ optional_indices{ true, { 0 }, { 0 } },
                                       optional_indices{ false, {}, {} },
                                       optional_indices{ false, { 2 }, { 2 } },
                                       optional_indices{ false, {}, {} },
                                       optional_indices{ false, {}, {} },
                                       optional_indices{ false, {}, {} },
                                       optional_indices{ false, {}, {} },
                                       optional_indices{ true, { 1 }, { 1 } },
                                       optional_indices{ false, {}, {} } }),
              expected_indices_tuple({ optional_indices{ true, { 2 }, { 0 } },
                                       optional_indices{ false, {}, {} },
                                       optional_indices{ false, { 0 }, { 0 } },
                                       optional_indices{ false, {}, {} },
                                       optional_indices{ false, {}, {} },
                                       optional_indices{ false, {}, {} },
                                       optional_indices{ false, {}, {} },
                                       optional_indices{ true, { 3 }, { 1 } },
                                       optional_indices{ false, {}, {} } }) });

        std::tie(res, descr) = check_ring_multiple_topologies<topology, process_group_context>(
            pg_comm->process_device_topology, params.thread_ids, expected_thread_results);
        UT_ASSERT(res, descr);
    }
}
} // namespace topology_suite
