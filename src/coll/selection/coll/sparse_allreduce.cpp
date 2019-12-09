#include "coll/selection/selection.hpp"

template<>
std::map<ccl_coll_sparse_allreduce_algo,
         std::string> ccl_algorithm_selector_helper<ccl_coll_sparse_allreduce_algo>::algo_names =
  { 
    std::make_pair(ccl_coll_sparse_allreduce_basic, "basic"),
    std::make_pair(ccl_coll_sparse_allreduce_size, "size"),
    std::make_pair(ccl_coll_sparse_allreduce_mask, "mask"),
    std::make_pair(ccl_coll_sparse_allreduce_3_allgatherv, "allgather")
  };

ccl_algorithm_selector<ccl_coll_sparse_allreduce>::ccl_algorithm_selector()
{
    insert(main_table, 0, CCL_SELECTION_MAX_COLL_SIZE, ccl_coll_sparse_allreduce_basic);
    insert(fallback_table, 0, CCL_SELECTION_MAX_COLL_SIZE, ccl_coll_sparse_allreduce_basic);
}

template<>
bool ccl_algorithm_selector_helper<ccl_coll_sparse_allreduce_algo>::is_direct(ccl_coll_sparse_allreduce_algo algo)
{
    return false;
}

template<>
bool ccl_algorithm_selector_helper<ccl_coll_sparse_allreduce_algo>::can_use(ccl_coll_sparse_allreduce_algo algo,
                                                                     const ccl_selector_param& param,
                                                                     const ccl_selection_table_t<ccl_coll_sparse_allreduce_algo>& table)
{
	CCL_THROW_IF_NOT(table.size() == 2,
		"CCL sparse_allreduce doesn't support algorithm selection for multiple size ranges, ",
		" please specify the single algorithm for the whole range");

    return true;
}

CCL_SELECTION_DEFINE_HELPER_METHODS(ccl_coll_sparse_allreduce_algo, ccl_coll_sparse_allreduce,
                                    env_data.sparse_allreduce_algo_raw, 0);
