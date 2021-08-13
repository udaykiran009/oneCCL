#pragma once

#include <iterator>
#include <sstream>

#include "coll/algorithms/algorithms.hpp"
#include "exec/exec.hpp"

inline bool ccl_can_use_topo_ring_algo(const ccl_selector_param& param) {
    if ((param.ctype != ccl_coll_allreduce) && (param.ctype != ccl_coll_bcast) &&
        (param.ctype != ccl_coll_reduce)) {
        return false;
    }

    bool is_sycl_buf = false;
    bool is_l0_backend = false;

#ifdef CCL_ENABLE_SYCL
    is_sycl_buf = param.is_sycl_buf;
#ifdef MULTI_GPU_SUPPORT
    if (param.stream && param.stream->get_backend() == sycl::backend::level_zero)
        is_l0_backend = true;
#endif // MULTI_GPU_SUPPORT
#endif // CCL_ENABLE_SYCL

    if ((param.comm->size() != 2) ||
        (param.comm->size() != ccl::global_data::get().executor->get_local_proc_count()) ||
        (!param.stream || param.stream->get_type() != stream_type::gpu) || is_sycl_buf ||
        !is_l0_backend || ccl::global_data::env().enable_fusion ||
        ccl::global_data::env().enable_unordered_coll ||
        (ccl::global_data::env().priority_mode != ccl_priority_none) ||
        (ccl::global_data::env().worker_count != 1)) {
        return false;
    }

    return true;
}

template <typename algo_group_type>
struct ccl_algorithm_selector_helper {
    static bool can_use(algo_group_type algo,
                        const ccl_selector_param& param,
                        const ccl_selection_table_t<algo_group_type>& table);
    static bool is_direct(algo_group_type algo);
    static const std::string& get_str_to_parse();
    static ccl_coll_type get_coll_id();
    static size_t get_count(const ccl_selector_param& param);
    static algo_group_type algo_from_str(const std::string& str);
    static const std::string& algo_to_str(algo_group_type algo);

    static std::map<algo_group_type, std::string> algo_names;
};

template <typename algo_group_type>
const std::string& ccl_coll_algorithm_to_str(algo_group_type algo) {
    return ccl_algorithm_selector_helper<algo_group_type>::algo_to_str(algo);
}

#define CCL_SELECTION_DEFINE_HELPER_METHODS(algo_group_type, coll_id, env_str, count_expr) \
    template <> \
    const std::string& ccl_algorithm_selector_helper<algo_group_type>::get_str_to_parse() { \
        return env_str; \
    } \
    template <> \
    ccl_coll_type ccl_algorithm_selector_helper<algo_group_type>::get_coll_id() { \
        return coll_id; \
    } \
    template <> \
    size_t ccl_algorithm_selector_helper<algo_group_type>::get_count( \
        const ccl_selector_param& param) { \
        return count_expr; \
    } \
    template <> \
    algo_group_type ccl_algorithm_selector_helper<algo_group_type>::algo_from_str( \
        const std::string& str) { \
        for (const auto& name : algo_names) { \
            if (!str.compare(name.second)) { \
                return name.first; \
            } \
        } \
        std::stringstream sstream; \
        std::for_each(algo_names.begin(), \
                      algo_names.end(), \
                      [&](const std::pair<algo_group_type, std::string>& p) { \
                          sstream << p.second << "\n"; \
                      }); \
        CCL_THROW( \
            "unknown algorithm name '", str, "'\n", "supported algorithms:\n", sstream.str()); \
    } \
    template <> \
    const std::string& ccl_algorithm_selector_helper<algo_group_type>::algo_to_str( \
        algo_group_type algo) { \
        auto it = algo_names.find(algo); \
        if (it != algo_names.end()) \
            return it->second; \
        static const std::string unknown("unknown"); \
        return unknown; \
    }

template <>
std::map<ccl_coll_allgatherv_algo, std::string>
    ccl_algorithm_selector_helper<ccl_coll_allgatherv_algo>::algo_names;

template <>
std::map<ccl_coll_allreduce_algo, std::string>
    ccl_algorithm_selector_helper<ccl_coll_allreduce_algo>::algo_names;

template <>
std::map<ccl_coll_barrier_algo, std::string>
    ccl_algorithm_selector_helper<ccl_coll_barrier_algo>::algo_names;

template <>
std::map<ccl_coll_bcast_algo, std::string>
    ccl_algorithm_selector_helper<ccl_coll_bcast_algo>::algo_names;

template <>
std::map<ccl_coll_reduce_algo, std::string>
    ccl_algorithm_selector_helper<ccl_coll_reduce_algo>::algo_names;

template <>
std::map<ccl_coll_reduce_scatter_algo, std::string>
    ccl_algorithm_selector_helper<ccl_coll_reduce_scatter_algo>::algo_names;

template <>
std::map<ccl_coll_sparse_allreduce_algo, std::string>
    ccl_algorithm_selector_helper<ccl_coll_sparse_allreduce_algo>::algo_names;
