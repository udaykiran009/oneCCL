#ifndef GLOBAL_H
#define GLOBAL_H

#include "comm.h"
#include "exec.h"
#include "utils.h"

#define MLSL_TAG_UNDEFINED (-1)
#define MLSL_TAG_FIRST     (0)

struct mlsl_global_data
{
    int tag_ub;
    mlsl_comm *comm;
    mlsl_executor *executor;
} __attribute__ ((aligned (CACHELINE_SIZE)));

typedef struct mlsl_global_data mlsl_global_data;

extern mlsl_global_data global_data;

#endif /* GLOBAL_H */
