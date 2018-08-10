#ifndef ENV_H
#define ENV_H

#include "utils.h"

struct mlsl_env_data
{
    int log_level;
    int sched_dump;
    int worker_count;
    int *worker_affinity;
} __attribute__ ((aligned (CACHELINE_SIZE)));

typedef struct mlsl_env_data mlsl_env_data;

extern mlsl_env_data env_data;

void mlsl_env_parse();
void mlsl_env_print();
void mlsl_env_free();
int mlsl_env_2_int(const char* env_name, int* val);
int mlsl_env_parse_affinity();
int mlsl_env_print_affinity();

#endif /* ENV_H */
