#pragma once
#include <initializer_list>
#include "../topology_utils.hpp"

static ccl::device_index_type dev_0(0, 0, ccl::unused_index_value);
static ccl::device_index_type dev_1(0, 1, ccl::unused_index_value);
static ccl::device_index_type dev_2(0, 2, ccl::unused_index_value);
static ccl::device_index_type dev_3(0, 3, ccl::unused_index_value);
static ccl::device_index_type dev_4(0, 4, ccl::unused_index_value);
static ccl::device_index_type dev_5(0, 5, ccl::unused_index_value);
static ccl::device_index_type dev_6(0, 6, ccl::unused_index_value);
static ccl::device_index_type dev_7(0, 7, ccl::unused_index_value);

namespace unit_test {
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
            auto merged =
                allied_process_group_ring_topology::merge_allied_nodes_in_colored_plain_graphs(
                    out,
                    cluster_indices,
                    process_index,
                    process_count,
                    overrided_colored_cluster_graphs_to_replace,
                    ping);
            auto* test_ctx = dynamic_cast<stub::process_context*>(&context);
            if (test_ctx && !overrided_colored_merged_cluster_graphs_to_check.empty()) {
                test_ctx->set_collect_cluster_colored_plain_graphs(
                    overrided_colored_merged_cluster_graphs_to_check);
            }

            return merged;
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
} // namespace unit_test
