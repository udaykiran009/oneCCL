#pragma once

#include "coll/coll.hpp"
#include "common/utils/utils.hpp"
#include "common/utils/yield.hpp"

#include <vector>

/* TODO: rework logging code */
constexpr const char* CCL_LOG_LEVEL = "CCL_LOG_LEVEL";
constexpr const char* CCL_SCHED_DUMP = "CCL_SCHED_DUMP";

constexpr const char* CCL_WORKER_COUNT = "CCL_WORKER_COUNT";
constexpr const char* CCL_WORKER_AFFINITY = "CCL_WORKER_AFFINITY";
constexpr const char* CCL_WORKER_OFFLOAD = "CCL_WORKER_OFFLOAD";

constexpr const char* CCL_ALLGATHERV = "CCL_ALLGATHERV";
constexpr const char* CCL_ALLREDUCE = "CCL_ALLREDUCE";
constexpr const char* CCL_BARRIER = "CCL_BARRIER";
constexpr const char* CCL_BCAST = "CCL_BCAST";
constexpr const char* CCL_REDUCE = "CCL_REDUCE";
constexpr const char* CCL_SPARSE_ALLREDUCE = "CCL_SPARSE_ALLREDUCE";
constexpr const char* CCL_UNORDERED_COLL = "CCL_UNORDERED_COLL";
constexpr const char* CCL_ALLGATHERV_IOV = "CCL_ALLGATHERV_IOV";

constexpr const char* CCL_FUSION = "CCL_FUSION";
constexpr const char* CCL_FUSION_BYTES_THRESHOLD = "CCL_FUSION_BYTES_THRESHOLD";
constexpr const char* CCL_FUSION_COUNT_THRESHOLD = "CCL_FUSION_COUNT_THRESHOLD";
constexpr const char* CCL_FUSION_CHECK_URGENT = "CCL_FUSION_CHECK_URGENT";
constexpr const char* CCL_FUSION_CYCLE_MS = "CCL_FUSION_CYCLE_MS";

constexpr const char* CCL_RMA = "CCL_RMA";
constexpr const char* CCL_PRIORITY = "CCL_PRIORITY";
constexpr const char* CCL_SPIN_COUNT = "CCL_SPIN_COUNT";
constexpr const char* CCL_YIELD = "CCL_YIELD";
constexpr const char* CCL_MAX_SHORT_SIZE = "CCL_MAX_SHORT_SIZE";
constexpr const char* CCL_CACHE_KEY = "CCL_CACHE_KEY";
constexpr const char* CCL_ATL_TRANSPORT = "CCL_ATL_TRANSPORT";

enum ccl_priority_mode
{
    ccl_priority_none,
    ccl_priority_direct,
    ccl_priority_lifo,

    ccl_priority_last_value
};

enum ccl_cache_key
{
    ccl_cache_key_full,
    ccl_cache_key_match_id,

    ccl_cache_key_last_value
};

struct alignas(CACHELINE_SIZE) ccl_env_data
{
    int log_level;
    int sched_dump;

    size_t worker_count;
    int worker_offload;
    std::vector<size_t> worker_affinity;

    ccl_coll_allgatherv_algo allgatherv_algo;
    ccl_coll_allreduce_algo allreduce_algo;
    ccl_coll_barrier_algo barrier_algo;
    ccl_coll_bcast_algo bcast_algo;
    ccl_coll_reduce_algo reduce_algo;
    ccl_coll_sparse_allreduce_algo sparse_allreduce_algo;

    int enable_unordered_coll;
    int enable_allgatherv_iov;

    int enable_fusion;
    int fusion_bytes_threshold;
    int fusion_count_threshold;
    int fusion_check_urgent;
    float fusion_cycle_ms;

    int enable_rma;
    ccl_priority_mode priority_mode;
    size_t spin_count;
    ccl_yield_type yield_type;
    size_t max_short_size;
    ccl_cache_key cache_key;
};

extern ccl_env_data env_data;

int ccl_env_2_int(const char* env_name, int& val);
int ccl_env_2_size_t(const char* env_name, size_t& val);
int ccl_env_2_float(const char* env_name, float& val);

void ccl_env_parse();
void ccl_env_print();

int ccl_env_parse_atl_transport();
int ccl_env_parse_worker_affinity();
int ccl_env_parse_allgatherv_algo();
int ccl_env_parse_allreduce_algo();
int ccl_env_parse_barrier_algo();
int ccl_env_parse_bcast_algo();
int ccl_env_parse_reduce_algo();
int ccl_env_parse_priority_mode();
int ccl_env_parse_yield_type();
int ccl_env_parse_cache_key();

const char* ccl_priority_mode_to_str(ccl_priority_mode type);
const char* ccl_cache_key_to_str(ccl_cache_key key);
