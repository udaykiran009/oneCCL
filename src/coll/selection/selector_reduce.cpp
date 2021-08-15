#include "coll/selection/selection.hpp"

template <>
std::map<ccl_coll_reduce_algo, std::string>
    ccl_algorithm_selector_helper<ccl_coll_reduce_algo>::algo_names = {
        std::make_pair(ccl_coll_reduce_direct, "direct"),
        std::make_pair(ccl_coll_reduce_rabenseifner, "rabenseifner"),
        std::make_pair(ccl_coll_reduce_tree, "tree"),
        std::make_pair(ccl_coll_reduce_double_tree, "double_tree"),
        std::make_pair(ccl_coll_reduce_topo_ring, "topo_ring")
    };

ccl_algorithm_selector<ccl_coll_reduce>::ccl_algorithm_selector() {
#if defined(CCL_ENABLE_SYCL) && defined(MULTI_GPU_SUPPORT)
    insert(main_table, 0, CCL_SELECTION_MAX_COLL_SIZE, ccl_coll_reduce_topo_ring);
#else // CCL_ENABLE_SYCL && MULTI_GPU_SUPPORT
    if (ccl::global_data::env().atl_transport == ccl_atl_ofi) {
        insert(main_table, 0, CCL_SELECTION_MAX_COLL_SIZE, ccl_coll_reduce_tree);
    }
    else if (ccl::global_data::env().atl_transport == ccl_atl_mpi) {
        insert(main_table, 0, CCL_SELECTION_MAX_COLL_SIZE, ccl_coll_reduce_direct);
    }
#endif // CCL_ENABLE_SYCL && MULTI_GPU_SUPPORT

    insert(fallback_table, 0, CCL_SELECTION_MAX_COLL_SIZE, ccl_coll_reduce_tree);
}

template <>
bool ccl_algorithm_selector_helper<ccl_coll_reduce_algo>::is_direct(ccl_coll_reduce_algo algo) {
    return (algo == ccl_coll_reduce_direct) ? true : false;
}

template <>
bool ccl_algorithm_selector_helper<ccl_coll_reduce_algo>::can_use(
    ccl_coll_reduce_algo algo,
    const ccl_selector_param& param,
    const ccl_selection_table_t<ccl_coll_reduce_algo>& table) {
    bool can_use = true;

    if (algo == ccl_coll_reduce_rabenseifner && (int)param.count < param.comm->pof2())
        can_use = false;
    else if (algo == ccl_coll_reduce_direct &&
             (ccl::global_data::env().atl_transport == ccl_atl_ofi))
        can_use = false;
    else if (algo == ccl_coll_reduce_topo_ring && !ccl_can_use_topo_ring_algo(param))
        can_use = false;

    return can_use;
}

CCL_SELECTION_DEFINE_HELPER_METHODS(ccl_coll_reduce_algo,
                                    ccl_coll_reduce,
                                    ccl::global_data::env().reduce_algo_raw,
                                    param.count);
