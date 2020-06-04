#include "coll/selection/selection.hpp"

template<>
std::map<ccl_coll_alltoallv_algo,
         std::string> ccl_algorithm_selector_helper<ccl_coll_alltoallv_algo>::algo_names =
  {
    std::make_pair(ccl_coll_alltoallv_direct, "direct"),
    std::make_pair(ccl_coll_alltoallv_naive, "naive")
  };

ccl_algorithm_selector<ccl_coll_alltoallv>::ccl_algorithm_selector()
{
    if (ccl::global_data::env().atl_transport == ccl_atl_ofi)
        insert(main_table, 0, CCL_SELECTION_MAX_COLL_SIZE, ccl_coll_alltoallv_naive);
    else if (ccl::global_data::env().atl_transport == ccl_atl_mpi)
        insert(main_table, 0, CCL_SELECTION_MAX_COLL_SIZE, ccl_coll_alltoallv_direct);

    insert(fallback_table, 0, CCL_SELECTION_MAX_COLL_SIZE, ccl_coll_alltoallv_naive);
}

template<>
bool ccl_algorithm_selector_helper<ccl_coll_alltoallv_algo>::is_direct(ccl_coll_alltoallv_algo algo)
{
    return (algo == ccl_coll_alltoallv_direct) ? true : false;
}

template<>
bool ccl_algorithm_selector_helper<ccl_coll_alltoallv_algo>::can_use(ccl_coll_alltoallv_algo algo,
                                                               const ccl_selector_param& param,
                                                               const ccl_selection_table_t<ccl_coll_alltoallv_algo>& table)
{
    return true;
}

CCL_SELECTION_DEFINE_HELPER_METHODS(ccl_coll_alltoallv_algo, ccl_coll_alltoallv,
                                    ccl::global_data::env().alltoallv_algo_raw, 0);
