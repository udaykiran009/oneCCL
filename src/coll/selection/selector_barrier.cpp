#include "coll/selection/selection.hpp"

template <>
std::map<ccl_coll_barrier_algo, std::string>
    ccl_algorithm_selector_helper<ccl_coll_barrier_algo>::algo_names = {
        std::make_pair(ccl_coll_barrier_direct, "direct"),
        std::make_pair(ccl_coll_barrier_ring, "ring")
    };

ccl_algorithm_selector<ccl_coll_barrier>::ccl_algorithm_selector() {
    if (ccl::global_data::env().atl_transport == ccl_atl_ofi)
        insert(main_table, 0, CCL_SELECTION_MAX_COLL_SIZE, ccl_coll_barrier_ring);
    else if (ccl::global_data::env().atl_transport == ccl_atl_mpi)
        insert(main_table, 0, CCL_SELECTION_MAX_COLL_SIZE, ccl_coll_barrier_direct);

    insert(fallback_table, 0, CCL_SELECTION_MAX_COLL_SIZE, ccl_coll_barrier_ring);
}

template <>
bool ccl_algorithm_selector_helper<ccl_coll_barrier_algo>::can_use(
    ccl_coll_barrier_algo algo,
    const ccl_selector_param& param,
    const ccl_selection_table_t<ccl_coll_barrier_algo>& table) {
    bool can_use = true;

    if (algo == ccl_coll_barrier_direct && (ccl::global_data::env().atl_transport == ccl_atl_ofi))
        can_use = false;

    return can_use;
}

CCL_SELECTION_DEFINE_HELPER_METHODS(ccl_coll_barrier_algo,
                                    ccl_coll_barrier,
                                    ccl::global_data::env().barrier_algo_raw,
                                    0);
