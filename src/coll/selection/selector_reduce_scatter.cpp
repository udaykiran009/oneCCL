#include "coll/selection/selection.hpp"

template <>
std::map<ccl_coll_reduce_scatter_algo, std::string>
    ccl_algorithm_selector_helper<ccl_coll_reduce_scatter_algo>::algo_names = {
        std::make_pair(ccl_coll_reduce_scatter_direct, "direct"),
        std::make_pair(ccl_coll_reduce_scatter_ring, "ring"),
        std::make_pair(ccl_coll_reduce_scatter_topo_a2a, "topo_a2a"),
    };

ccl_algorithm_selector<ccl_coll_reduce_scatter>::ccl_algorithm_selector() {
    if (ccl::global_data::env().atl_transport == ccl_atl_ofi)
        insert(main_table, 0, CCL_SELECTION_MAX_COLL_SIZE, ccl_coll_reduce_scatter_ring);
    else if (ccl::global_data::env().atl_transport == ccl_atl_mpi)
        insert(main_table, 0, CCL_SELECTION_MAX_COLL_SIZE, ccl_coll_reduce_scatter_direct);

    insert(fallback_table, 0, CCL_SELECTION_MAX_COLL_SIZE, ccl_coll_reduce_scatter_ring);
}

template <>
bool ccl_algorithm_selector_helper<ccl_coll_reduce_scatter_algo>::can_use(
    ccl_coll_reduce_scatter_algo algo,
    const ccl_selector_param& param,
    const ccl_selection_table_t<ccl_coll_reduce_scatter_algo>& table) {
    bool can_use = true;

    if (algo == ccl_coll_reduce_scatter_topo_a2a && !ccl_can_use_topo_a2a_algo(param)) {
        can_use = false;
    }
    else if (algo == ccl_coll_reduce_scatter_direct &&
             (ccl::global_data::env().atl_transport == ccl_atl_ofi))
        can_use = false;

    return can_use;
}

CCL_SELECTION_DEFINE_HELPER_METHODS(ccl_coll_reduce_scatter_algo,
                                    ccl_coll_reduce_scatter,
                                    ccl::global_data::env().reduce_scatter_algo_raw,
                                    param.count);