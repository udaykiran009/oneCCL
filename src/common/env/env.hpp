#pragma once

#include "common/utils/utils.hpp"

constexpr const char* MLSL_LOG_LEVEL = "MLSL_LOG_LEVEL";
constexpr const char* MLSL_SCHED_DUMP = "MLSL_SCHED_DUMP";
constexpr const char* MLSL_WORKER_COUNT = "MLSL_WORKER_COUNT";
constexpr const char* MLSL_WORKER_OFFLOAD = "MLSL_WORKER_OFFLOAD";
constexpr const char* MLSL_OUT_OF_ORDER_SUPPORT = "MLSL_OUT_OF_ORDER_SUPPORT";
constexpr const char* MLSL_ENABLE_RMA = "MLSL_ENABLE_RMA";
constexpr const char* MLSL_ENABLE_FUSION = "MLSL_ENABLE_FUSION";
constexpr const char* MLSL_FUSION_BYTES_THRESHOLD = "MLSL_FUSION_BYTES_THRESHOLD";
constexpr const char* MLSL_FUSION_COUNT_THRESHOLD = "MLSL_FUSION_COUNT_THRESHOLD";
constexpr const char* MLSL_FUSION_CHECK_URGENT = "MLSL_FUSION_CHECK_URGENT";
constexpr const char* MLSL_FUSION_CYCLE_MS = "MLSL_FUSION_CYCLE_MS";
constexpr const char* MLSL_PRIORITY_MODE = "MLSL_PRIORITY_MODE";
constexpr const char* MLSL_WORKER_AFFINITY = "MLSL_WORKER_AFFINITY";
constexpr const char* MLSL_ALLREDUCE_ALGO = "MLSL_ALLREDUCE_ALGO";
constexpr const char* MLSL_BCAST_ALGO = "MLSL_BCAST_ALGO";
constexpr const char* MLSL_REDUCE_ALGO = "MLSL_REDUCE_ALGO";
constexpr const char* MLSL_VECTOR_ALLGATHERV = "MLSL_VECTOR_ALLGATHERV";

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
    mlsl_allreduce_algo_ring_rma     = 3,
    mlsl_allreduce_algo_double_tree  = 4
};

enum mlsl_bcast_algo
{
    mlsl_bcast_algo_ring          = 0,
    mlsl_bcast_algo_double_tree   = 1
};

enum mlsl_reduce_algo
{
    mlsl_reduce_algo_tree          = 0,
    mlsl_reduce_algo_double_tree   = 1
};


enum mlsl_sparse_allreduce_algo
{
    mlsl_sparse_allreduce_algo_basic = 0,
    mlsl_sparse_allreduce_algo_mask  = 1
};

//todo: set/get methods
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
    mlsl_bcast_algo bcast_algo;
    mlsl_reduce_algo reduce_algo;
    mlsl_sparse_allreduce_algo sparse_allreduce_algo;
    int enable_rma;
    int enable_fusion;
    int fusion_bytes_threshold;
    int fusion_count_threshold;
    int fusion_check_urgent;
    float fusion_cycle_ms;
    int vector_allgatherv; /* TODO: figure out if there're options better than env var to control this feature */
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
int mlsl_env_parse_bcast_algo();
int mlsl_env_parse_reduce_algo();
const char *mlsl_allreduce_algo_to_str(mlsl_allreduce_algo algo);
