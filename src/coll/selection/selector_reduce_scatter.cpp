#include "coll/selection/selection.hpp"

template <>
std::map<ccl_coll_reduce_scatter_algo, std::string>
    ccl_algorithm_selector_helper<ccl_coll_reduce_scatter_algo>::algo_names = {
        std::make_pair(ccl_coll_reduce_scatter_ring, "ring"),
    };

ccl_algorithm_selector<ccl_coll_reduce_scatter>::ccl_algorithm_selector() {
    insert(main_table, 0, CCL_SELECTION_MAX_COLL_SIZE, ccl_coll_reduce_scatter_ring);
    insert(fallback_table, 0, CCL_SELECTION_MAX_COLL_SIZE, ccl_coll_reduce_scatter_ring);
}

template <>
bool ccl_algorithm_selector_helper<ccl_coll_reduce_scatter_algo>::is_direct(
    ccl_coll_reduce_scatter_algo algo) {
    return false;
}

template <>
bool ccl_algorithm_selector_helper<ccl_coll_reduce_scatter_algo>::can_use(
    ccl_coll_reduce_scatter_algo algo,
    const ccl_selector_param& param,
    const ccl_selection_table_t<ccl_coll_reduce_scatter_algo>& table) {
    return true;
}

CCL_SELECTION_DEFINE_HELPER_METHODS(ccl_coll_reduce_scatter_algo,
                                    ccl_coll_reduce_scatter,
                                    ccl::global_data::env().reduce_scatter_algo_raw,
                                    param.count);