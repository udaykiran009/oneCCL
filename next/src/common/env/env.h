#ifndef ENV_H
#define ENV_H

#include "utils.h"

struct mlsl_env_data
{
    int log_level;
    int dump_sched;
} __attribute__ ((aligned (CACHELINE_SIZE)));

typedef struct mlsl_env_data mlsl_env_data;

extern mlsl_env_data env_data;

void mlsl_env_parse();
void mlsl_env_print();
int mlsl_env_2_int(const char* env_name, int* val);

#endif /* ENV_H */
