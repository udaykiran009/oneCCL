#include "coll/selection/selection.hpp"

template<>
std::map<ccl_coll_bcast_algo,
         std::string> ccl_algorithm_selector_helper<ccl_coll_bcast_algo>::algo_names =
  { 
    std::make_pair(ccl_coll_bcast_direct, "direct"),
    std::make_pair(ccl_coll_bcast_ring, "ring"),
    std::make_pair(ccl_coll_bcast_double_tree, "double_tree"),
    std::make_pair(ccl_coll_bcast_naive, "naive")
  };

ccl_algorithm_selector<ccl_coll_bcast>::ccl_algorithm_selector()
{
    if (env_data.atl_transport == ccl_atl_ofi)
    {
        insert(main_table, 0, CCL_SELECTION_MAX_COLL_SIZE, ccl_coll_bcast_naive);
        insert(main_table, 0, CCL_BCAST_SHORT_MSG_SIZE, ccl_coll_bcast_double_tree);
    }
    else if (env_data.atl_transport == ccl_atl_mpi)
        insert(main_table, 0, CCL_SELECTION_MAX_COLL_SIZE, ccl_coll_bcast_direct);

    insert(fallback_table, 0, CCL_SELECTION_MAX_COLL_SIZE, ccl_coll_bcast_naive);
}

template<>
bool ccl_algorithm_selector_helper<ccl_coll_bcast_algo>::is_direct(ccl_coll_bcast_algo algo)
{
    return (algo == ccl_coll_bcast_direct) ? true : false;
}

template<>
bool ccl_algorithm_selector_helper<ccl_coll_bcast_algo>::can_use(ccl_coll_bcast_algo algo,
                                                          const ccl_selector_param& param,
                                                          const ccl_selection_table_t<ccl_coll_bcast_algo>& table)
{
    bool can_use = true;

    if (env_data.enable_unordered_coll &&
        algo == ccl_coll_bcast_double_tree)
    {
        /* TODO: stabilize double_tree bcast for unordered_coll case */ 
        can_use = false;
    }

    return can_use;
}

CCL_SELECTION_DEFINE_HELPER_METHODS(ccl_coll_bcast_algo, ccl_coll_bcast,
                                    env_data.bcast_algo_raw, param.count);
