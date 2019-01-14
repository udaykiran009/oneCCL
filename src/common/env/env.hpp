#pragma once

#include "common/utils/utils.hpp"

enum mlsl_priority_mode
{
    mlsl_priority_none   = 0,
    mlsl_priority_direct = 1,
    mlsl_priority_lifo   = 2
};

enum mlsl_allreduce_algo
{
    mlsl_allreduce_algo_rabenseifner = 0,
    mlsl_allreduce_algo_starlike     = 1
};

struct mlsl_env_data
{
    int log_level;
    int sched_dump;
    int worker_count;
    int worker_offload;
    int out_of_order_support;
    int *worker_affinity;
    mlsl_priority_mode priority_mode;
    mlsl_allreduce_algo allreduce_algo;
} __attribute__ ((aligned (CACHELINE_SIZE)));


extern mlsl_env_data env_data;

void mlsl_env_parse();
void mlsl_env_print();
void mlsl_env_free();
int mlsl_env_2_int(const char* env_name, int* val);
int mlsl_env_parse_priority_mode();
int mlsl_env_parse_affinity();
int mlsl_env_print_affinity();
int mlsl_env_parse_allreduce_algo();
