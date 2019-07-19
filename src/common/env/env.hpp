#pragma once

#include "common/utils/utils.hpp"
#include <vector>

constexpr const char* ICCL_LOG_LEVEL = "ICCL_LOG_LEVEL";
constexpr const char* ICCL_SCHED_DUMP = "ICCL_SCHED_DUMP";
constexpr const char* ICCL_WORKER_COUNT = "ICCL_WORKER_COUNT";
constexpr const char* ICCL_WORKER_OFFLOAD = "ICCL_WORKER_OFFLOAD";
constexpr const char* ICCL_OUT_OF_ORDER_SUPPORT = "ICCL_OUT_OF_ORDER_SUPPORT";
constexpr const char* ICCL_ENABLE_RMA = "ICCL_ENABLE_RMA";
constexpr const char* ICCL_ENABLE_FUSION = "ICCL_ENABLE_FUSION";
constexpr const char* ICCL_FUSION_BYTES_THRESHOLD = "ICCL_FUSION_BYTES_THRESHOLD";
constexpr const char* ICCL_FUSION_COUNT_THRESHOLD = "ICCL_FUSION_COUNT_THRESHOLD";
constexpr const char* ICCL_FUSION_CHECK_URGENT = "ICCL_FUSION_CHECK_URGENT";
constexpr const char* ICCL_FUSION_CYCLE_MS = "ICCL_FUSION_CYCLE_MS";
constexpr const char* ICCL_PRIORITY_MODE = "ICCL_PRIORITY_MODE";
constexpr const char* ICCL_WORKER_AFFINITY = "ICCL_WORKER_AFFINITY";
constexpr const char* ICCL_ALLREDUCE_ALGO = "ICCL_ALLREDUCE_ALGO";
constexpr const char* ICCL_BCAST_ALGO = "ICCL_BCAST_ALGO";
constexpr const char* ICCL_BARRIER_ALGO = "ICCL_BARRIER_ALGO";
constexpr const char* ICCL_REDUCE_ALGO = "ICCL_REDUCE_ALGO";
constexpr const char* ICCL_VECTOR_ALLGATHERV = "ICCL_VECTOR_ALLGATHERV";
constexpr const char* ICCL_FULL_CACHE_KEY = "ICCL_FULL_CACHE_KEY";

enum iccl_priority_mode
{
    iccl_priority_none = 0,
    iccl_priority_direct = 1,
    iccl_priority_lifo = 2
};

enum iccl_allreduce_algo
{
    iccl_allreduce_algo_rabenseifner = 0,
    iccl_allreduce_algo_starlike     = 1,
    iccl_allreduce_algo_ring         = 2,
    iccl_allreduce_algo_ring_rma     = 3,
    iccl_allreduce_algo_double_tree  = 4,
    iccl_allreduce_algo_direct       = 5
};

enum iccl_allgatherv_algo
{
    iccl_allgatherv_algo_naive       = 0,
    iccl_allgatherv_algo_direct      = 1
};

enum iccl_bcast_algo
{
    iccl_bcast_algo_ring             = 0,
    iccl_bcast_algo_double_tree      = 1,
    iccl_bcast_algo_direct           = 2
};

enum iccl_reduce_algo
{
    iccl_reduce_algo_tree            = 0,
    iccl_reduce_algo_double_tree     = 1,
    iccl_reduce_algo_direct          = 2
};

enum iccl_barrier_algo
{
    iccl_barrier_algo_ring           = 0,
    iccl_barrier_algo_direct         = 1
};

enum iccl_sparse_allreduce_algo
{
    iccl_sparse_allreduce_algo_basic = 0,
    iccl_sparse_allreduce_algo_mask  = 1
};

//todo: set/get methods
struct alignas(CACHELINE_SIZE) iccl_env_data
{
    int log_level;
    int sched_dump;
    size_t worker_count;
    int worker_offload;
    int out_of_order_support;
    std::vector<int> worker_affinity;
    iccl_priority_mode priority_mode;
    iccl_allreduce_algo allreduce_algo;
    iccl_allgatherv_algo allgatherv_algo;
    iccl_bcast_algo bcast_algo;
    iccl_reduce_algo reduce_algo;
    iccl_barrier_algo barrier_algo;
    iccl_sparse_allreduce_algo sparse_allreduce_algo;
    int enable_rma;
    int enable_fusion;
    int fusion_bytes_threshold;
    int fusion_count_threshold;
    int fusion_check_urgent;
    float fusion_cycle_ms;
    int vector_allgatherv; /* TODO: figure out if there're options better than env var to control this feature */
    int full_cache_key;
};

extern iccl_env_data env_data;

void iccl_env_parse();
void iccl_env_print();
int iccl_env_2_int(const char* env_name,
                   int& val);
int iccl_env_2_size_t(const char* env_name,
                      size_t& val);
int iccl_env_2_float(const char* env_name,
                     float& val);
int iccl_env_parse_priority_mode();
int iccl_env_parse_affinity();
int iccl_env_print_affinity();
int iccl_env_parse_allreduce_algo();
int iccl_env_parse_bcast_algo();
int iccl_env_parse_reduce_algo();
int iccl_env_parse_barrier_algo();
const char *iccl_allreduce_algo_to_str(iccl_allreduce_algo algo);
const char *iccl_allgatherv_algo_to_str(iccl_allgatherv_algo algo);
const char *iccl_bcast_algo_to_str(iccl_bcast_algo algo);
const char *iccl_reduce_algo_to_str(iccl_reduce_algo algo);
const char *iccl_barrier_algo_to_str(iccl_barrier_algo algo);

