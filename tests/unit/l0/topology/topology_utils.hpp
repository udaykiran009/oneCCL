#pragma once
#include <map>
#include <vector>
#include <string>

#include "hw_topology_fixture.hpp"
#include "common/comm/l0/devices/devices_declaration.hpp"

namespace utils {

std::pair<bool, std::string> check_adj_matrix(
    const native::detail::adjacency_matrix& matrix,
    const ccl::device_indices_type& idx,
    native::detail::cross_device_rating expected_self,
    native::detail::cross_device_rating expected_other = 0) {
    //check matrix
    std::pair<bool, std::string> ret{ true, "" };
    for (const auto& lhs_pair : matrix) {
        const ccl::device_index_type& i = lhs_pair.first;
        const native::detail::adjacency_list& rhs_list = lhs_pair.second;
        for (const auto& rhs_pair : rhs_list) {
            const ccl::device_index_type& j = rhs_pair.first;
            native::detail::cross_device_rating rating = rhs_pair.second;
            if (idx.find(i) != idx.end() && idx.find(j) != idx.end()) {
                native::detail::cross_device_rating compared_value = expected_other;
                if (i == j) {
                    compared_value = expected_self;
                }

                (void)compared_value; //TODO
                if (rating == 0 /*TODO compared_value*/) {
                    if (ret.first) {
                        ret.first = false;
                        ret.second = "Unexpected matrix value at: ";
                    }
                    ret.second += std::string("[") + ccl::to_string(i) + "x" + ccl::to_string(j) +
                                  "] == " + std::to_string(rating);
                }
            }
        }
    }
    return ret;
}

using optional = std::pair<bool, size_t>;
using expected_tuple = std::tuple<optional, optional, optional, optional, optional>;

using optional_indices =
    std::tuple<bool, std::set<size_t>, std::vector<ccl::index_type>>; //<check, index, rank>
using expected_indices_tuple = std::vector<optional_indices>;

template <class device_t>
void set_control_indices(std::vector<expected_indices_tuple>& sequence,
                         size_t thread_id,
                         const optional_indices& data) {
    if (sequence.size() < thread_id + 1) {
        sequence.resize(thread_id + 1);
    }

    if (sequence[thread_id].size() < device_t::type_idx() + 1) {
        sequence[thread_id].resize(device_t::type_idx() + 1);
    }

    sequence[thread_id][device_t::type_idx()] = data;
}

template <class device_t, ccl::group_split_type topology, ccl::device_topology_type class_id>
std::pair<bool, std::string> check_device(
    const native::specific_indexed_device_storage& device_group,
    const expected_indices_tuple& exp_idx) {
    std::pair<bool, std::string> res{ true, "" };
    native::indexed_device_container<device_t> cont = std::get<device_t::type_idx()>(device_group);

    if (exp_idx.size() <= device_t::type_idx()) {
        res.first = true;
        res.second = "check disabled";
        return res;
    }

    bool should_check = false;
    std::set<size_t> indexes;
    std::vector<ccl::index_type> ranks;
    std::tie(should_check, indexes, ranks) = exp_idx[device_t::type_idx()];
    if (should_check) {
        if (cont.size() != indexes.size()) {
            res.first = false;
            res.second = std::string(device_t::name()) +
                         " failed! Expected: " + std::to_string(indexes.size()) +
                         ", got: " + std::to_string(cont.size()) + "\n";
        }

        //check indices
        auto rank_it = ranks.begin();
        for (size_t idx : indexes) {
            auto checked_it = cont.find(idx);
            if (checked_it == cont.end()) {
                res.first = false;
                res.second = std::string(device_t::name()) +
                             " cannot find expected idx: " + std::to_string(idx) + "\n";
                return res;
            }

            //check ranks, if needed
            if (!ranks.empty()) {
                const auto& addr = checked_it->second->template get_comm_data<topology, class_id>();
                if (addr.rank != *rank_it) {
                    res.first = false;
                    res.second = std::string(device_t::name()) +
                                 " Invalid rank or size: " + addr.to_string() +
                                 " for device by index: " + std::to_string(idx) +
                                 ", expected: " + std::to_string(*rank_it) + ")\n";
                    return res;
                }
                ++rank_it;
            }
        }
    }
    return res;
}

template <ccl::group_split_type topology,
          ccl::device_topology_type class_id = ccl::device_topology_type::ring>
std::pair<bool, std::string> check_topology(
    const std::unique_ptr<native::specific_indexed_device_storage>& device_group,
    const expected_indices_tuple& values) {
    using namespace native;
    bool ret = true, tmp = true;
    std::string descr, str_tmp;

    {
        std::tie(tmp, str_tmp) =
            check_device<ccl_gpu_comm, topology, class_id>(*device_group, values);
        ret &= tmp;
        if (!tmp) {
            descr += str_tmp;
        }
    }

    {
        std::tie(tmp, str_tmp) =
            check_device<ccl_virtual_gpu_comm, topology, class_id>(*device_group, values);
        ret &= tmp;
        if (!tmp) {
            descr += str_tmp;
        }
    }

    {
        std::tie(tmp, str_tmp) =
            check_device<ccl_thread_comm<ccl_gpu_comm>, topology, class_id>(*device_group, values);
        ret &= tmp;
        if (!tmp) {
            descr += str_tmp;
        }
    }
    {
        std::tie(tmp, str_tmp) =
            check_device<ccl_thread_comm<ccl_virtual_gpu_comm>, topology, class_id>(*device_group,
                                                                                    values);
        ret &= tmp;
        if (!tmp) {
            descr += str_tmp;
        }
    }
    {
        std::tie(tmp, str_tmp) =
            check_device<ccl_ipc_source_gpu_comm<ccl_gpu_comm>, topology, class_id>(*device_group,
                                                                                    values);
        ret &= tmp;
        if (!tmp) {
            descr += str_tmp;
        }
    }
    {
        std::tie(tmp, str_tmp) =
            check_device<ccl_ipc_source_gpu_comm<ccl_virtual_gpu_comm>, topology, class_id>(
                *device_group, values);
        ret &= tmp;
        if (!tmp) {
            descr += str_tmp;
        }
    }
    {
        std::tie(tmp, str_tmp) =
            check_device<ccl_ipc_gpu_comm, topology, class_id>(*device_group, values);
        ret &= tmp;
        if (!tmp) {
            descr += str_tmp;
        }
    }
    {
        std::tie(tmp, str_tmp) =
            check_device<ccl_gpu_scaleup_proxy<ccl_gpu_comm>, topology, class_id>(*device_group,
                                                                                  values);
        ret &= tmp;
        if (!tmp) {
            descr += str_tmp;
        }
    }
    {
        std::tie(tmp, str_tmp) =
            check_device<ccl_gpu_scaleup_proxy<ccl_virtual_gpu_comm>, topology, class_id>(
                *device_group, values);
        ret &= tmp;
        if (!tmp) {
            descr += str_tmp;
        }
    }
    return { ret, descr };
}

template <ccl::group_split_type topology,
          ccl::device_topology_type class_id,
          class ctx,
          class tuple>
std::pair<bool, std::string> check_ring_multiple_topologies(
    const std::map<size_t, tuple>& topologies,
    const std::vector<size_t>& sequencial_indices,
    const std::vector<expected_indices_tuple>& sequencial_values,
    bool is_torn_apart = false,
    size_t ring_index = 0) {
    bool res = true;
    std::string descr;

    using topologies_t =
        native::device_group_community_holder<topology, SUPPORTED_TOPOLOGY_CLASSES_DECL_LIST>;

    for (size_t thread_id : sequencial_indices) {
        //Check single topology
        bool tmp = true;
        std::string str_tmp;
        const topologies_t& all_top_types = topologies.find(thread_id)->second;

        const auto& top_type =
            (not is_torn_apart)
                ? all_top_types.template get_community<class_id>().get_topology(ring_index)
                : all_top_types.template get_community<class_id>().get_additiona_topology(
                      ring_index);
        if (!top_type) {
            res = false;
            descr += std::string("Thread: ") + std::to_string(thread_id) +
                     " - there is no topology: " + std::to_string((int)topology);
            break;
        }

        const auto& devices_ptr = top_type->devices;
        if (!devices_ptr) {
            res = false;
            descr += std::string("Thread: ") + std::to_string(thread_id) +
                     " - there are no devices for topology: " + std::to_string((int)topology);
            break;
        }

        std::tie(tmp, str_tmp) =
            check_topology<topology, class_id>(devices_ptr, sequencial_values[thread_id]);
        if (!tmp) {
            res = false;

            native::detail::printer<topology, class_id> p;
            ccl_tuple_for_each(*top_type->devices, p);
            descr += std::string("Thread: ") + std::to_string(thread_id) + "\n" + p.to_string() +
                     "\nFailed with: " + str_tmp + "\n";
        }
    }
    return { res, descr };
}

std::pair<bool, std::string> check_id_ring(native::detail::adjacency_matrix matrix,
                                           const ccl::device_indices_type& idx,
                                           const native::detail::plain_graph& expected) {
    bool ret = true;
    std::string err;

    native::detail::plain_graph id_ring;
    try {
        id_ring = native::detail::graph_resolver(matrix, idx);

        if (id_ring != expected) {
            throw std::runtime_error("Id Ring has unexpected structure");
        }
    }
    catch (const std::exception& ex) {
        ret = false;
        std::stringstream ss;
        ss << "Ring: ";
        for (const auto& id : id_ring) {
            ss << id << ", ";
        }

        ss << "\nExpected: ";
        for (const auto& id : expected) {
            ss << id << ", ";
        }
        err = ss.str();
        err += std::string("\nError: ") + ex.what() + "\n";
    }
    return { ret, err };
}

std::pair<bool, std::string> check_multiple_graph_ring(
    const native::detail::plain_graph_list& graphs_to_test,
    const native::detail::plain_graph_list& expected) {
    bool ret = true;
    std::string err;

    try {
        if (graphs_to_test.size() != expected.size()) {
            throw std::runtime_error("Different graph sizes");
        }

        size_t size = expected.size();
        for (size_t i = 0; i < size; i++) {
            auto graph_it = graphs_to_test.begin();
            std::advance(graph_it, i);
            auto graph = *graph_it; //copy

            auto exp_it = expected.begin();
            std::advance(exp_it, i);
            auto expected_graph = *exp_it; //copy

            std::sort(graph.begin(), graph.end());
            std::sort(expected_graph.begin(), expected_graph.end());

            std::vector<ccl::device_index_type> diff;
            std::set_symmetric_difference(graph.begin(),
                                          graph.end(),
                                          expected_graph.begin(),
                                          expected_graph.end(),
                                          std::back_inserter(diff));
            if (!diff.empty()) {
                std::stringstream ss;
                for (const auto& idx : diff) {
                    ss << idx << ", ";
                }

                throw std::runtime_error(std::string("Unexpected graph content for graph index: ") +
                                         std::to_string(i) + "\nUnexpected indices: " + ss.str());
            }
        }
    }
    catch (const std::exception& ex) {
        ret = false;
        std::stringstream ss;
        ss << "Tested graphs: ";
        for (const auto& graph : graphs_to_test) {
            ss << "{ ";
            for (const auto& idx : graph) {
                ss << idx << ",";
            }
            ss << "}\n";
        }

        ss << "\nExpected graphs: ";
        for (const auto& graph : expected) {
            ss << "{ ";
            for (const auto& idx : graph) {
                ss << idx << ",";
            }
            ss << "}\n";
        }
        err = ss.str();
        err += std::string("\nError: ") + ex.what() + "\n";
    }
    return { ret, err };
}

native::detail::cross_device_rating test_custom_p2p_ping(
    const native::ccl_device& lhs,
    const native::ccl_device& rhs,
    const native::detail::adjacency_matrix& desired_matrix) {
    const auto& lhs_path = lhs.get_device_path();
    auto lhs_it = desired_matrix.find(lhs_path);
    if (lhs_it == desired_matrix.end()) {
        throw std::runtime_error(
            std::string(__FUNCTION__) +
            " - desired_matrix doesn't contains lhs index: " + ccl::to_string(lhs_path));
    }

    const auto& rhs_path = rhs.get_device_path();
    auto rhs_it = lhs_it->second.find(rhs_path);
    if (rhs_it == lhs_it->second.end()) {
        throw std::runtime_error(
            std::string(__FUNCTION__) + " - desired_matrix doesn't contains rhs index: " +
            ccl::to_string(rhs_path) + ", for lhs device by: " + ccl::to_string(lhs_path));
    }
    return rhs_it->second;
}

native::detail::cross_device_rating all_p2p_accessible(const native::ccl_device&,
                                                       const native::ccl_device&) {
    return 1;
}

native::detail::cross_device_rating nobody_p2p_accessible(const native::ccl_device&,
                                                          const native::ccl_device&) {
    return 0;
}

void dump_global_colored_graph(std::ostream& out,
                               const native::detail::global_colored_plain_graphs& graphs) {
    using namespace native::detail;
    for (const auto& process_data : graphs) {
        const colored_plain_graph_list& graphs = process_data.second;

        out << "process: " << process_data.first << ", graphs: " << graphs.size() << std::endl;
        size_t graph_counter = 0;
        for (const colored_plain_graph& colored_graph : graphs) {
            out << graph_counter++ << ":\n";
            out << to_string(colored_graph) << std::endl;
        }
    }
}
} // namespace utils
