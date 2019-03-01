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
    mlsl_allreduce_algo_starlike     = 1,
    mlsl_allreduce_algo_ring         = 2,
    mlsl_allreduce_algo_ring_rma     = 3
};

enum mlsl_sparse_allreduce_algo
{
    mlsl_sparse_allreduce_algo_basic = 0,
    mlsl_sparse_allreduce_algo_mask  = 1
};

struct alignas(CACHELINE_SIZE) mlsl_env_data
{
    int log_level;
    int sched_dump;
    int worker_count;
    int worker_offload;
    int out_of_order_support;
    int *worker_affinity;
    mlsl_priority_mode priority_mode;
    mlsl_allreduce_algo allreduce_algo;
    mlsl_sparse_allreduce_algo sparse_allreduce_algo;
    int enable_rma;
    int enable_fusion;
    int fusion_bytes_threshold;
    int fusion_count_threshold;
    int fusion_check_urgent;
    float fusion_cycle_ms;
};

extern mlsl_env_data env_data;

void mlsl_env_parse();
void mlsl_env_print();
void mlsl_env_free();
int mlsl_env_2_int(const char* env_name, int* val);
int mlsl_env_2_float(const char* env_name, float* val);
int mlsl_env_parse_priority_mode();
int mlsl_env_parse_affinity();
int mlsl_env_print_affinity();
int mlsl_env_parse_allreduce_algo();
const char *mlsl_allreduce_algo_to_str(mlsl_allreduce_algo algo);
