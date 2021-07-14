#include "coll/selection/selection.hpp"

template <>
std::map<ccl_coll_alltoallv_algo, std::string>
    ccl_algorithm_selector_helper<ccl_coll_alltoallv_algo>::algo_names = {
        std::make_pair(ccl_coll_alltoallv_direct, "direct"),
        std::make_pair(ccl_coll_alltoallv_naive, "naive"),
        std::make_pair(ccl_coll_alltoallv_scatter, "scatter"),
        std::make_pair(ccl_coll_alltoallv_scatter_barrier, "scatter_barrier")
    };

ccl_algorithm_selector<ccl_coll_alltoallv>::ccl_algorithm_selector() {
    if (ccl::global_data::env().atl_transport == ccl_atl_ofi) {
        insert(main_table, 0, CCL_ALLTOALL_MEDIUM_MSG_SIZE, ccl_coll_alltoallv_scatter);
    }
    else if (ccl::global_data::env().atl_transport == ccl_atl_mpi) {
        insert(main_table, 0, CCL_ALLTOALL_MEDIUM_MSG_SIZE, ccl_coll_alltoallv_direct);
    }

    insert(main_table,
           CCL_ALLTOALL_MEDIUM_MSG_SIZE + 1,
           CCL_SELECTION_MAX_COLL_SIZE,
           ccl_coll_alltoallv_scatter_barrier);

    insert(fallback_table, 0, CCL_SELECTION_MAX_COLL_SIZE, ccl_coll_alltoallv_scatter_barrier);
}

template <>
bool ccl_algorithm_selector_helper<ccl_coll_alltoallv_algo>::is_direct(
    ccl_coll_alltoallv_algo algo) {
    return (algo == ccl_coll_alltoallv_direct) ? true : false;
}

template <>
bool ccl_algorithm_selector_helper<ccl_coll_alltoallv_algo>::can_use(
    ccl_coll_alltoallv_algo algo,
    const ccl_selector_param& param,
    const ccl_selection_table_t<ccl_coll_alltoallv_algo>& table) {
    bool can_use = true;

    if (param.is_vector_buf && algo != ccl_coll_alltoallv_scatter_barrier)
        can_use = false;
    else if (algo == ccl_coll_alltoallv_direct &&
             (ccl::global_data::env().atl_transport == ccl_atl_ofi))
        can_use = false;

    return can_use;
}

CCL_SELECTION_DEFINE_HELPER_METHODS(ccl_coll_alltoallv_algo,
                                    ccl_coll_alltoallv,
                                    ccl::global_data::env().alltoallv_algo_raw,
                                    0);
