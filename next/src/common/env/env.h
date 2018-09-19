#ifndef ENV_H
#define ENV_H

#include "utils.h"

enum mlsl_priority_mode
{
    mlsl_priority_none   = 0,
    mlsl_priority_direct = 1,
    mlsl_priority_lifo   = 2
};
typedef enum mlsl_priority_mode mlsl_priority_mode;

struct mlsl_env_data
{
    int log_level;
    int sched_dump;
    int worker_count;
    int worker_offload;
    int *worker_affinity;
    mlsl_priority_mode priority_mode;
} __attribute__ ((aligned (CACHELINE_SIZE)));

typedef struct mlsl_env_data mlsl_env_data;

extern mlsl_env_data env_data;

void mlsl_env_parse();
void mlsl_env_print();
void mlsl_env_free();
int mlsl_env_2_int(const char* env_name, int* val);
int mlsl_env_parse_priority_mode();
int mlsl_env_parse_affinity();
int mlsl_env_print_affinity();

#endif /* ENV_H */
