#ifndef GLOBAL_H
#define GLOBAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "common/comm/comm.h"
#include "common/utils/utils.h"
#include "exec/exec.h"
#include "parallelizer/parallelizer.h"
#include "sched/sched_cache.h"

struct mlsl_global_data
{
    mlsl_comm *comm;
    mlsl_executor *executor;
    mlsl_sched_cache *sched_cache;
    mlsl_parallelizer *parallelizer;
    mlsl_coll_attr_t *default_coll_attr;
} __attribute__ ((aligned (CACHELINE_SIZE)));

typedef struct mlsl_global_data mlsl_global_data;

extern mlsl_global_data global_data;

#ifdef __cplusplus
}
#endif

#endif /* GLOBAL_H */
