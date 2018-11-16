#pragma once

#include "common/comm/comm.hpp"
#include "common/utils/utils.hpp"
#include "exec/exec.hpp"
#include "parallelizer/parallelizer.hpp"
#include "sched/sched_cache.hpp"

struct mlsl_global_data
{
    mlsl_comm *comm;
    mlsl_executor *executor;
    mlsl_sched_cache *sched_cache;
    mlsl_parallelizer *parallelizer;
    mlsl_coll_attr_t *default_coll_attr;
} __attribute__ ((aligned (CACHELINE_SIZE)));

extern mlsl_global_data global_data;
