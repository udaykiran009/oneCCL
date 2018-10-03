#ifndef PARALLELIZER_H
#define PARALLELIZER_H

#include "sched.h"

struct mlsl_parallelizer
{
	size_t partition_count;
};

typedef struct mlsl_parallelizer mlsl_parallelizer;

extern mlsl_parallelizer *global_parallelizer;

mlsl_status_t mlsl_parallelizer_create(size_t partition_count, mlsl_parallelizer **parallelizer);
mlsl_status_t mlsl_parallelizer_free(mlsl_parallelizer *parallelizer);
mlsl_status_t mlsl_parallelizer_process(mlsl_parallelizer *parallelizer, mlsl_sched *sched,
                                        mlsl_sched ***scheds, size_t *sched_count);

#endif /* PARALLELIZER_H */
