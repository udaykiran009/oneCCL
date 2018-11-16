#include "common/global/global.hpp"

mlsl_global_data global_data = { .comm = NULL,
                                 .executor = NULL,
                                 .sched_cache = NULL,
                                 .parallelizer = NULL,
                                 .default_coll_attr = NULL
                               };
