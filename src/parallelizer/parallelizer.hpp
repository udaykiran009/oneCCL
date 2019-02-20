#pragma once

#include "sched/sched.hpp"

struct mlsl_parallelizer
{
    size_t partition_count;
};

extern mlsl_parallelizer *global_parallelizer;

mlsl_status_t mlsl_parallelizer_create(size_t partition_count, mlsl_parallelizer **parallelizer);
mlsl_status_t mlsl_parallelizer_free(mlsl_parallelizer *parallelizer);
mlsl_status_t mlsl_parallelizer_process(mlsl_parallelizer* parallelizer,
                                        mlsl_sched* sched);
