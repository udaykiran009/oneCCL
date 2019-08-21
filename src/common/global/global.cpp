#include "common/comm/atl_tag.hpp"
#include "common/comm/comm_id_storage.hpp"
#include "common/global/global.hpp"
#include "common/utils/tree.hpp"
#include "common/stream/stream.hpp"
#include "coll/selection/selection.hpp"
#include "exec/exec.hpp"
#include "fusion/fusion.hpp"
#include "parallelizer/parallelizer.hpp"
#include "sched/sched_cache.hpp"
#include "unordered_coll/unordered_coll.hpp"

ccl_global_data global_data{};
thread_local bool ccl_global_data::is_worker_thread = false;
