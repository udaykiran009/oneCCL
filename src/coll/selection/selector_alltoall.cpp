#include "coll/selection/selection.hpp"

template <>
std::map<ccl_coll_alltoall_algo, std::string>
    ccl_algorithm_selector_helper<ccl_coll_alltoall_algo>::algo_names = {
        std::make_pair(ccl_coll_alltoall_direct, "direct"),
        std::make_pair(ccl_coll_alltoall_naive, "naive"),
        std::make_pair(ccl_coll_alltoall_scatter, "scatter")
    };

ccl_algorithm_selector<ccl_coll_alltoall>::ccl_algorithm_selector() {
    insert(main_table, 0, CCL_SELECTION_MAX_COLL_SIZE, ccl_coll_alltoall_scatter);
    if (ccl::global_data::env().atl_transport == ccl_atl_mpi) {
        insert(main_table, 0, CCL_ALLTOALL_MEDIUM_MSG_SIZE, ccl_coll_alltoall_direct);
    }

    insert(fallback_table, 0, CCL_SELECTION_MAX_COLL_SIZE, ccl_coll_alltoall_naive);
}

template <>
bool ccl_algorithm_selector_helper<ccl_coll_alltoall_algo>::can_use(
    ccl_coll_alltoall_algo algo,
    const ccl_selector_param& param,
    const ccl_selection_table_t<ccl_coll_alltoall_algo>& table) {
    bool can_use = true;

    if (algo == ccl_coll_alltoall_direct && (ccl::global_data::env().atl_transport == ccl_atl_ofi))
        can_use = false;

    return can_use;
}

CCL_SELECTION_DEFINE_HELPER_METHODS(ccl_coll_alltoall_algo,
                                    ccl_coll_alltoall,
                                    ccl::global_data::env().alltoall_algo_raw,
                                    param.count);
