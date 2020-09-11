#include "coll/coll.hpp"
#include "common/comm/l0/modules/modules_source_data.hpp"
namespace native {

using specific_modules_source_data_storage =
    modules_src_container<ccl_coll_allgatherv, ccl_coll_allreduce>;
}
