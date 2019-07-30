#pragma once

#include "common/utils/utils.hpp"
#include "common/utils/yield.hpp"

#include <vector>

constexpr const char* CCL_SPIN_COUNT = "CCL_SPIN_COUNT";
constexpr const char* CCL_YIELD_TYPE = "CCL_YIELD_TYPE";
constexpr const char* CCL_MAX_SHORT_SIZE = "CCL_MAX_SHORT_SIZE";
constexpr const char* CCL_LOG_LEVEL = "CCL_LOG_LEVEL";
constexpr const char* CCL_SCHED_DUMP = "CCL_SCHED_DUMP";
constexpr const char* CCL_WORKER_COUNT = "CCL_WORKER_COUNT";
constexpr const char* CCL_WORKER_OFFLOAD = "CCL_WORKER_OFFLOAD";
constexpr const char* CCL_OUT_OF_ORDER_SUPPORT = "CCL_OUT_OF_ORDER_SUPPORT";
constexpr const char* CCL_ENABLE_RMA = "CCL_ENABLE_RMA";
constexpr const char* CCL_ENABLE_FUSION = "CCL_ENABLE_FUSION";
constexpr const char* CCL_FUSION_BYTES_THRESHOLD = "CCL_FUSION_BYTES_THRESHOLD";
constexpr const char* CCL_FUSION_COUNT_THRESHOLD = "CCL_FUSION_COUNT_THRESHOLD";
constexpr const char* CCL_FUSION_CHECK_URGENT = "CCL_FUSION_CHECK_URGENT";
constexpr const char* CCL_FUSION_CYCLE_MS = "CCL_FUSION_CYCLE_MS";
constexpr const char* CCL_PRIORITY_MODE = "CCL_PRIORITY_MODE";
constexpr const char* CCL_WORKER_AFFINITY = "CCL_WORKER_AFFINITY";
constexpr const char* CCL_ALLGATHERV_ALGO = "CCL_ALLGATHERV_ALGO";
constexpr const char* CCL_ALLREDUCE_ALGO = "CCL_ALLREDUCE_ALGO";
constexpr const char* CCL_BARRIER_ALGO = "CCL_BARRIER_ALGO";
constexpr const char* CCL_BCAST_ALGO = "CCL_BCAST_ALGO";
constexpr const char* CCL_REDUCE_ALGO = "CCL_REDUCE_ALGO";
constexpr const char* CCL_VECTOR_ALLGATHERV = "CCL_VECTOR_ALLGATHERV";
constexpr const char* CCL_FULL_CACHE_KEY = "CCL_FULL_CACHE_KEY";

enum ccl_priority_mode
{
    ccl_priority_none = 0,
    ccl_priority_direct = 1,
    ccl_priority_lifo = 2,

    ccl_priority_last_value
};

enum ccl_allgatherv_algo
{
    ccl_allgatherv_algo_naive       = 0,
    ccl_allgatherv_algo_direct      = 1,
    ccl_allgatherv_algo_multi_bcast = 2,

    ccl_allgatherv_algo_last_value
};

enum ccl_allreduce_algo
{
    ccl_allreduce_algo_rabenseifner = 0,
    ccl_allreduce_algo_starlike     = 1,
    ccl_allreduce_algo_ring         = 2,
    ccl_allreduce_algo_ring_rma     = 3,
    ccl_allreduce_algo_double_tree  = 4,
    ccl_allreduce_algo_direct       = 5,

    ccl_allreduce_algo_last_value
};

enum ccl_barrier_algo
{
    ccl_barrier_algo_ring           = 0,
    ccl_barrier_algo_direct         = 1,

    ccl_barrier_algo_last_value
};

enum ccl_bcast_algo
{
    ccl_bcast_algo_ring             = 0,
    ccl_bcast_algo_double_tree      = 1,
    ccl_bcast_algo_direct           = 2,

    ccl_bcast_algo_last_value
};

enum ccl_reduce_algo
{
    ccl_reduce_algo_tree            = 0,
    ccl_reduce_algo_double_tree     = 1,
    ccl_reduce_algo_direct          = 2,

    ccl_reduce_algo_last_value
};

enum ccl_sparse_allreduce_algo
{
    ccl_sparse_allreduce_algo_basic = 0,
    ccl_sparse_allreduce_algo_mask  = 1,

    ccl_sparse_allreduce_algo_last_value
};

struct alignas(CACHELINE_SIZE) ccl_env_data
{
    size_t spin_count;
    ccl_yield_type yield_type;
    size_t max_short_size;
    int log_level;
    int sched_dump;
    size_t worker_count;
    int worker_offload;
    int out_of_order_support;
    std::vector<int> worker_affinity;
    ccl_priority_mode priority_mode;
    ccl_allgatherv_algo allgatherv_algo;
    ccl_allreduce_algo allreduce_algo;
    ccl_barrier_algo barrier_algo;
    ccl_bcast_algo bcast_algo;
    ccl_reduce_algo reduce_algo;
    ccl_sparse_allreduce_algo sparse_allreduce_algo;
    int enable_rma;
    int enable_fusion;
    int fusion_bytes_threshold;
    int fusion_count_threshold;
    int fusion_check_urgent;
    float fusion_cycle_ms;
    int vector_allgatherv;
    int full_cache_key;
};

extern ccl_env_data env_data;

void ccl_env_parse();
void ccl_env_print();
int ccl_env_2_int(const char* env_name,
                   int& val);
int ccl_env_2_size_t(const char* env_name,
                      size_t& val);
int ccl_env_2_float(const char* env_name,
                     float& val);

int ccl_env_parse_priority_mode();
int ccl_env_parse_affinity();
int ccl_env_parse_yield_type();

int ccl_env_parse_allgatherv_algo();
int ccl_env_parse_allreduce_algo();
int ccl_env_parse_barrier_algo();
int ccl_env_parse_bcast_algo();
int ccl_env_parse_reduce_algo();

const char *ccl_allgatherv_algo_to_str(ccl_allgatherv_algo algo);
const char *ccl_allreduce_algo_to_str(ccl_allreduce_algo algo);
const char *ccl_barrier_algo_to_str(ccl_barrier_algo algo);
const char *ccl_bcast_algo_to_str(ccl_bcast_algo algo);
const char *ccl_reduce_algo_to_str(ccl_reduce_algo algo);
