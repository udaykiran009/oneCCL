#include "common/global/global.hpp"
#include "common/comm/comm_id_storage.hpp"
#include "exec/exec.hpp"
#include "sched/sched_cache.hpp"
#include "parallelizer/parallelizer.hpp"
#include "out_of_order/ooo_match.hpp"
#include "fusion/fusion.hpp"

mlsl_global_data global_data{};
thread_local bool mlsl_global_data::is_worker_thread = false;
