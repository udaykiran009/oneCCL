#include "coll/selection/selection.hpp"

template <>
std::map<ccl_coll_reduce_algo, std::string>
    ccl_algorithm_selector_helper<ccl_coll_reduce_algo>::algo_names = {
        std::make_pair(ccl_coll_reduce_direct, "direct"),
        std::make_pair(ccl_coll_reduce_rabenseifner, "rabenseifner"),
        std::make_pair(ccl_coll_reduce_tree, "tree"),
        std::make_pair(ccl_coll_reduce_double_tree, "double_tree"),
#ifdef CCL_ENABLE_SYCL
        std::make_pair(ccl_coll_reduce_topo, "topo")
#endif // CCL_ENABLE_SYCL
    };

ccl_algorithm_selector<ccl_coll_reduce>::ccl_algorithm_selector() {
#if defined(CCL_ENABLE_SYCL) && defined(CCL_ENABLE_ZE)
    insert(main_table, 0, CCL_SELECTION_MAX_COLL_SIZE, ccl_coll_reduce_topo);
#else // CCL_ENABLE_SYCL && CCL_ENABLE_ZE
    if (ccl::global_data::env().atl_transport == ccl_atl_ofi) {
        insert(main_table, 0, CCL_SELECTION_MAX_COLL_SIZE, ccl_coll_reduce_tree);
    }
    else if (ccl::global_data::env().atl_transport == ccl_atl_mpi) {
        insert(main_table, 0, CCL_SELECTION_MAX_COLL_SIZE, ccl_coll_reduce_direct);
    }
#endif // CCL_ENABLE_SYCL && CCL_ENABLE_ZE

    insert(fallback_table, 0, CCL_SELECTION_MAX_COLL_SIZE, ccl_coll_reduce_tree);
}

template <>
bool ccl_algorithm_selector_helper<ccl_coll_reduce_algo>::can_use(
    ccl_coll_reduce_algo algo,
    const ccl_selector_param& param,
    const ccl_selection_table_t<ccl_coll_reduce_algo>& table) {
    bool can_use = true;
	

    ccl_coll_algo algo_param;
    algo_param.reduce = algo;
    can_use = ccl_can_use_datatype(algo_param, param);
	

    if (algo == ccl_coll_reduce_rabenseifner && (int)param.count < param.comm->pof2()){
		
		can_use = false;
	}
    else if (algo == ccl_coll_reduce_direct &&
             (ccl::global_data::env().atl_transport == ccl_atl_ofi)){
			
			 can_use = false;
			 }
    else if (algo == ccl_coll_reduce_topo && !ccl_can_use_topo_algo(param)){
		can_use = false;
	}
	

    return can_use;
}

CCL_SELECTION_DEFINE_HELPER_METHODS(ccl_coll_reduce_algo,
                                    ccl_coll_reduce,
                                    ccl::global_data::env().reduce_algo_raw,
                                    param.count);
