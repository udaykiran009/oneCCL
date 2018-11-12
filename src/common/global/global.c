#include "common/global/global.h"

mlsl_global_data global_data = { .comm = NULL,
                                 .executor = NULL,
                                 .sched_cache = NULL,
                                 .parallelizer = NULL,
                                 .default_coll_attr = NULL
                               };
