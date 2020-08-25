#include "common/comm/comm_id_storage.hpp"
#include "common/global/global.hpp"



class ccl_executor{};
class ccl_parallelizer{};
class ccl_fusion_manager{};
class ccl_unordered_coll_manager{};
class ccl_allreduce_2d_builder{};
template<ccl_coll_type... registered_types_id>
class  ccl_algorithm_selector_wrapper{};

namespace ccl
{
global_data::global_data()
{
    comm_ids = std::unique_ptr<ccl_comm_id_storage>(new ccl_comm_id_storage(ccl_comm::max_comm_count));
}

global_data::~global_data()
{
    comm_ids.reset();
}

global_data& global_data::get()
{
    static global_data data;
    return data;
}

env_data& global_data::env()
{
    return get().env_object;
}
} /* namespace ccl */
