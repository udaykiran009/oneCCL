#include "coll/selection/selection.hpp"

template <>
std::map<ccl_coll_bcast_algo, std::string>
    ccl_algorithm_selector_helper<ccl_coll_bcast_algo>::algo_names = {
        std::make_pair(ccl_coll_bcast_direct, "direct"),
        std::make_pair(ccl_coll_bcast_ring, "ring"),
        std::make_pair(ccl_coll_bcast_double_tree, "double_tree"),
        std::make_pair(ccl_coll_bcast_naive, "naive"),
        std::make_pair(ccl_coll_bcast_topo_ring, "topo_ring")
    };

ccl_algorithm_selector<ccl_coll_bcast>::ccl_algorithm_selector() {
    if (ccl::global_data::env().atl_transport == ccl_atl_ofi) {
        insert(main_table, 0, CCL_SELECTION_MAX_COLL_SIZE, ccl_coll_bcast_naive);
        insert(main_table, 0, CCL_BCAST_SHORT_MSG_SIZE, ccl_coll_bcast_double_tree);
    }
    else if (ccl::global_data::env().atl_transport == ccl_atl_mpi)
        insert(main_table, 0, CCL_SELECTION_MAX_COLL_SIZE, ccl_coll_bcast_direct);

    insert(fallback_table, 0, CCL_SELECTION_MAX_COLL_SIZE, ccl_coll_bcast_naive);
}

template <>
bool ccl_algorithm_selector_helper<ccl_coll_bcast_algo>::is_direct(ccl_coll_bcast_algo algo) {
    return (algo == ccl_coll_bcast_direct) ? true : false;
}

template <>
bool ccl_algorithm_selector_helper<ccl_coll_bcast_algo>::can_use(
    ccl_coll_bcast_algo algo,
    const ccl_selector_param& param,
    const ccl_selection_table_t<ccl_coll_bcast_algo>& table) {
    bool can_use = true;
    bool is_sycl_buf = false;
    bool is_l0_backend = false;
#ifdef CCL_ENABLE_SYCL
    is_sycl_buf = param.is_sycl_buf;
#ifdef MULTI_GPU_SUPPORT
    if (param.stream && param.stream->get_backend() == sycl::backend::level_zero)
        is_l0_backend = true;
#endif // MULTI_GPU_SUPPORT
#endif // CCL_ENABLE_SYCL

    if (ccl::global_data::env().enable_unordered_coll && algo == ccl_coll_bcast_double_tree) {
        /* TODO: stabilize double_tree bcast for unordered_coll case */
        can_use = false;
    }
    else if (algo == ccl_coll_bcast_direct &&
             (ccl::global_data::env().atl_transport == ccl_atl_ofi))
        can_use = false;
    else if (algo == ccl_coll_bcast_topo_ring &&
             ((param.comm->size() != 2) ||
              (param.comm->size() != ccl::global_data::get().executor->get_local_proc_count()) ||
              (!param.stream || param.stream->get_type() != stream_type::gpu) || is_sycl_buf ||
              !is_l0_backend || ccl::global_data::env().enable_fusion ||
              ccl::global_data::env().enable_unordered_coll ||
              (ccl::global_data::env().priority_mode != ccl_priority_none) ||
              (ccl::global_data::env().worker_count != 1)))
        can_use = false;

    return can_use;
}

CCL_SELECTION_DEFINE_HELPER_METHODS(ccl_coll_bcast_algo,
                                    ccl_coll_bcast,
                                    ccl::global_data::env().bcast_algo_raw,
                                    param.count);
