#include "coll/selection/selection.hpp"

template <>
std::map<ccl_coll_allreduce_algo, std::string>
    ccl_algorithm_selector_helper<ccl_coll_allreduce_algo>::algo_names = {
        std::make_pair(ccl_coll_allreduce_direct, "direct"),
        std::make_pair(ccl_coll_allreduce_rabenseifner, "rabenseifner"),
        std::make_pair(ccl_coll_allreduce_starlike, "starlike"),
        std::make_pair(ccl_coll_allreduce_ring, "ring"),
        std::make_pair(ccl_coll_allreduce_ring_rma, "ring_rma"),
        std::make_pair(ccl_coll_allreduce_double_tree, "double_tree"),
        std::make_pair(ccl_coll_allreduce_recursive_doubling, "recursive_doubling"),
        std::make_pair(ccl_coll_allreduce_2d, "2d"),
        std::make_pair(ccl_coll_allreduce_topo_ring, "topo_ring"),
        std::make_pair(ccl_coll_allreduce_topo_a2a, "topo_a2a")
    };

ccl_algorithm_selector<ccl_coll_allreduce>::ccl_algorithm_selector() {
#if defined(CCL_ENABLE_SYCL) && defined(CCL_ENABLE_ZE)
    insert(main_table, 0, CCL_SELECTION_MAX_COLL_SIZE, ccl_coll_allreduce_topo_ring);
    // insert(main_table, 0, CCL_SELECTION_MAX_COLL_SIZE, ccl_coll_allreduce_topo_a2a);
#else // CCL_ENABLE_SYCL && CCL_ENABLE_ZE
    if (ccl::global_data::env().atl_transport == ccl_atl_ofi) {
        insert(main_table, 0, CCL_SELECTION_MAX_COLL_SIZE, ccl_coll_allreduce_ring);
        insert(main_table, 0, CCL_ALLREDUCE_SHORT_MSG_SIZE, ccl_coll_allreduce_recursive_doubling);
        insert(main_table,
               CCL_ALLREDUCE_SHORT_MSG_SIZE + 1,
               CCL_ALLREDUCE_MEDIUM_MSG_SIZE,
               ccl_coll_allreduce_starlike);
    }
    else if (ccl::global_data::env().atl_transport == ccl_atl_mpi)
        insert(main_table, 0, CCL_SELECTION_MAX_COLL_SIZE, ccl_coll_allreduce_direct);
#endif // CCL_ENABLE_SYCL && CCL_ENABLE_ZE

    insert(fallback_table, 0, CCL_SELECTION_MAX_COLL_SIZE, ccl_coll_allreduce_ring);
    insert(fallback_table, 0, CCL_ALLREDUCE_SHORT_MSG_SIZE, ccl_coll_allreduce_recursive_doubling);
}

template <>
bool ccl_algorithm_selector_helper<ccl_coll_allreduce_algo>::can_use(
    ccl_coll_allreduce_algo algo,
    const ccl_selector_param& param,
    const ccl_selection_table_t<ccl_coll_allreduce_algo>& table) {
    bool can_use = true;

    ccl_coll_algo algo_param;
    algo_param.allreduce = algo;
    can_use = ccl_can_use_datatype(algo_param, param);

    if (algo == ccl_coll_allreduce_rabenseifner &&
        static_cast<int>(param.count) < param.comm->pof2())
        can_use = false;
    else if (algo == ccl_coll_allreduce_ring_rma && !atl_base_comm::attr.out.enable_rma)
        can_use = false;
    else if (algo == ccl_coll_allreduce_starlike && !(param.count / param.comm->size()))
        can_use = false;
    else if (algo == ccl_coll_allreduce_2d &&
             (ccl::global_data::env().atl_transport == ccl_atl_mpi))
        can_use = false;
    else if (algo == ccl_coll_allreduce_direct &&
             (ccl::global_data::env().atl_transport == ccl_atl_ofi))
        can_use = false;
    else if (algo == ccl_coll_allreduce_topo_ring && !ccl_can_use_topo_ring_algo(param))
        can_use = false;
    else if (algo == ccl_coll_allreduce_topo_a2a && !ccl_can_use_topo_a2a_algo(param))
        can_use = false;

    return can_use;
}

CCL_SELECTION_DEFINE_HELPER_METHODS(ccl_coll_allreduce_algo,
                                    ccl_coll_allreduce,
                                    ccl::global_data::env().allreduce_algo_raw,
                                    param.count);
