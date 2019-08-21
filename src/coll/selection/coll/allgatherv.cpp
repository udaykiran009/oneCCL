#include "coll/selection/selection.hpp"

template<>
std::map<ccl_coll_allgatherv_algo,
         std::string> ccl_algorithm_selector_helper<ccl_coll_allgatherv_algo>::algo_names =
  {
    std::make_pair(ccl_coll_allgatherv_direct, "direct"),
    std::make_pair(ccl_coll_allgatherv_naive, "naive"),
    std::make_pair(ccl_coll_allgatherv_flat, "flat"),
    std::make_pair(ccl_coll_allgatherv_multi_bcast, "multi_bcast")
  };

ccl_algorithm_selector<ccl_coll_allgatherv>::ccl_algorithm_selector()
{
    if (env_data.atl_transport == ccl_atl_ofi)
        insert(main_table, 0, CCL_SELECTION_MAX_COLL_SIZE, ccl_coll_allgatherv_naive);
    else if (env_data.atl_transport == ccl_atl_mpi)
        insert(main_table, 0, CCL_SELECTION_MAX_COLL_SIZE, ccl_coll_allgatherv_multi_bcast);

    insert(fallback_table, 0, CCL_SELECTION_MAX_COLL_SIZE, ccl_coll_allgatherv_multi_bcast);
}

template<>
bool ccl_algorithm_selector_helper<ccl_coll_allgatherv_algo>::can_use(ccl_coll_allgatherv_algo algo,
                                                               const ccl_coll_param& param,
                                                               const ccl_selection_table_t<ccl_coll_allgatherv_algo>& table)
{
    bool can_use = true;

    if (env_data.enable_allgatherv_iov &&
        algo != ccl_coll_allgatherv_flat &&
        algo != ccl_coll_allgatherv_multi_bcast)
        can_use = false;

    return can_use;
}

CCL_SELECTION_DEFINE_HELPER_METHODS(ccl_coll_allgatherv_algo, ccl_coll_allgatherv, env_data.allgatherv_algo_raw,
  ({
      CCL_ASSERT(param.recv_counts);
      size_t count = 0;
      for (size_t idx = 0; idx < param.comm->size(); idx++)
      {
          count += param.recv_counts[idx];
      }
      count /= param.comm->size();
      count;
  }));
