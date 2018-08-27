#include "global.h"

mlsl_global_data global_data = { .tag_ub = 32768,
	                             .comm = NULL,
	                             .executor = NULL,
	                             .sched_cache = NULL,
	                             .parallelizer = NULL
	                           };
