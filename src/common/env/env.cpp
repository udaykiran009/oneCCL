#include "common/env/env.hpp"
#include "common/log/log.hpp"

#include <unistd.h>

ccl_env_data env_data =
{
    .log_level = static_cast<int>(ccl_log_level::ERROR),
    .sched_dump = 0,

    .worker_count = 1,
    .worker_offload = 1,
    .worker_affinity = std::vector<size_t>(),

    .allgatherv_algo = ccl_allgatherv_algo_multi_bcast,
    .allreduce_algo = ccl_allreduce_algo_direct,
    .barrier_algo = ccl_barrier_algo_direct,
    .bcast_algo = ccl_bcast_algo_direct,
    .reduce_algo = ccl_reduce_algo_direct,
    .sparse_allreduce_algo = ccl_sparse_allreduce_algo_basic,
    .enable_unordered_coll = 0,
    .enable_allgatherv_iov = 0,

    .enable_fusion = 0,
    .fusion_bytes_threshold = 16384,
    .fusion_count_threshold = 256,
    .fusion_check_urgent = 1,
    .fusion_cycle_ms = 0.2,
        
    .enable_rma = 0,
    .priority_mode = ccl_priority_none,
    .spin_count = 100,
    .yield_type = ccl_yield_pause,
    .max_short_size = 4096,
    .cache_key = ccl_cache_key_match_id
};

int ccl_env_2_int(const char* env_name, int& val)
{
    const char* val_ptr;
    val_ptr = getenv(env_name);
    if (val_ptr)
    {
        val = std::strtol(val_ptr, nullptr, 10);
        return 1;
    }
    return 0;
}

int ccl_env_2_size_t(const char* env_name, size_t& val)
{
    const char* val_ptr;
    val_ptr = getenv(env_name);
    if (val_ptr)
    {
        val = std::strtoul(val_ptr, nullptr, 10);
        return 1;
    }
    return 0;
}

int ccl_env_2_float(const char* env_name, float& val)
{
    const char* val_ptr;
    val_ptr = getenv(env_name);
    if (val_ptr)
    {
        val = std::strtof(val_ptr, nullptr);
        return 1;
    }
    return 0;
}

template<typename T>
void str_to_array(const char* input,
                  std::vector<T>& output,
                  char delimiter)
{
    std::stringstream ss(input);
    T temp{};
    while (ss >> temp)
    {
        output.push_back(temp);
        if (ss.peek() == delimiter)
        {
            ss.ignore();
        }
    }
}

void ccl_env_parse()
{
    ccl_env_2_int(CCL_LOG_LEVEL, env_data.log_level);
    ccl_logger::set_log_level(static_cast<ccl_log_level>(env_data.log_level));
    ccl_env_2_int(CCL_SCHED_DUMP, env_data.sched_dump);
    
    ccl_env_2_size_t(CCL_WORKER_COUNT, env_data.worker_count);
    CCL_THROW_IF_NOT(env_data.worker_count >= 1, "incorrect ", CCL_WORKER_COUNT, " ", env_data.worker_count);
    ccl_env_2_int(CCL_WORKER_OFFLOAD, env_data.worker_offload);
    ccl_env_parse_worker_affinity();

    ccl_env_parse_atl_transport();  
    ccl_env_parse_allgatherv_algo();
    ccl_env_parse_allreduce_algo();
    ccl_env_parse_barrier_algo();
    ccl_env_parse_bcast_algo();
    ccl_env_parse_reduce_algo();
    ccl_env_2_int(CCL_UNORDERED_COLL, env_data.enable_unordered_coll);
    ccl_env_2_int(CCL_ALLGATHERV_IOV, env_data.enable_allgatherv_iov);
    
    ccl_env_2_int(CCL_FUSION, env_data.enable_fusion);
    ccl_env_2_int(CCL_FUSION_BYTES_THRESHOLD, env_data.fusion_bytes_threshold);
    ccl_env_2_int(CCL_FUSION_COUNT_THRESHOLD, env_data.fusion_count_threshold);
    ccl_env_2_int(CCL_FUSION_CHECK_URGENT, env_data.fusion_check_urgent);
    ccl_env_2_float(CCL_FUSION_CYCLE_MS, env_data.fusion_cycle_ms);
    if (env_data.enable_fusion)
    {
        CCL_THROW_IF_NOT(env_data.fusion_bytes_threshold >= 1, "incorrect ",
                         CCL_FUSION_BYTES_THRESHOLD, " ", env_data.fusion_bytes_threshold);
        CCL_THROW_IF_NOT(env_data.fusion_count_threshold >= 1, "incorrect ",
                         CCL_FUSION_COUNT_THRESHOLD, " ", env_data.fusion_count_threshold);
    }

    ccl_env_2_int(CCL_RMA, env_data.enable_rma);
    ccl_env_parse_priority_mode();
    ccl_env_2_size_t(CCL_SPIN_COUNT, env_data.spin_count);
    ccl_env_parse_yield_type();
    ccl_env_2_size_t(CCL_MAX_SHORT_SIZE, env_data.max_short_size);
    ccl_env_parse_cache_key();
}

void ccl_env_print()
{
    LOG_INFO(CCL_LOG_LEVEL, ": ", env_data.log_level);
    LOG_INFO(CCL_SCHED_DUMP, ": ", env_data.sched_dump);
    
    LOG_INFO(CCL_WORKER_COUNT, ": ", env_data.worker_count);
    LOG_INFO(CCL_WORKER_OFFLOAD, ": ", env_data.worker_offload);
    for (size_t w_idx = 0; w_idx < env_data.worker_count; w_idx++)
    {
        LOG_INFO(CCL_WORKER_AFFINITY, ": worker: ", w_idx, ", processor: ", env_data.worker_affinity[w_idx]);
    }

    LOG_INFO(CCL_ALLGATHERV, ": ", ccl_allgatherv_algo_to_str(env_data.allgatherv_algo));
    LOG_INFO(CCL_ALLREDUCE, ": ", ccl_allreduce_algo_to_str(env_data.allreduce_algo));
    LOG_INFO(CCL_BARRIER, ": ",  ccl_barrier_algo_to_str(env_data.barrier_algo));
    LOG_INFO(CCL_BCAST, ": ", ccl_bcast_algo_to_str(env_data.bcast_algo));
    LOG_INFO(CCL_REDUCE, ": ", ccl_reduce_algo_to_str(env_data.reduce_algo));
    LOG_INFO(CCL_UNORDERED_COLL, ": ", env_data.enable_unordered_coll);
    LOG_INFO(CCL_ALLGATHERV_IOV, ": ", env_data.enable_allgatherv_iov);

    LOG_INFO(CCL_FUSION, ": ", env_data.enable_fusion);
    LOG_INFO(CCL_FUSION_BYTES_THRESHOLD, ": ", env_data.fusion_bytes_threshold);
    LOG_INFO(CCL_FUSION_COUNT_THRESHOLD, ": ", env_data.fusion_count_threshold);
    LOG_INFO(CCL_FUSION_CHECK_URGENT, ": ", env_data.fusion_check_urgent);
    LOG_INFO(CCL_FUSION_CYCLE_MS, ": ", env_data.fusion_cycle_ms);

    LOG_INFO(CCL_RMA, ": ", env_data.enable_rma);
    LOG_INFO(CCL_PRIORITY, ": ", ccl_priority_mode_to_str(env_data.priority_mode));
    LOG_INFO(CCL_SPIN_COUNT, ": ", env_data.spin_count);
    LOG_INFO(CCL_YIELD, ": ", ccl_yield_type_to_str(env_data.yield_type));
    LOG_INFO(CCL_MAX_SHORT_SIZE, ": ", env_data.max_short_size);
    LOG_INFO(CCL_CACHE_KEY, ": ", ccl_cache_key_to_str(env_data.cache_key));
}

constexpr const char* AVAILABLE_CORES_ENV = "I_MPI_PIN_INFO";

static int ccl_env_parse_auto_worker_affinity()
{
    char* available_cores = std::getenv(AVAILABLE_CORES_ENV);
    CCL_THROW_IF_NOT(available_cores && strlen(available_cores) != 0, "auto pinning requires ",
                      AVAILABLE_CORES_ENV, " env variable to be set");
    std::vector<size_t> cores;
    str_to_array(available_cores + 1, cores, ',');

    CCL_THROW_IF_NOT(env_data.worker_count <= cores.size(), "count of workers ", env_data.worker_count,
                     " exceeds the number of available cores ", cores.size());

    size_t ccl_cores_start = cores.size() - env_data.worker_count;
    for (size_t idx = 0; idx < env_data.worker_count; ++idx)
    {
        env_data.worker_affinity[idx] = cores[ccl_cores_start + idx];
    }
    return 1;
}

int ccl_env_parse_worker_affinity()
{
    int read_env = 0;
    size_t workers_per_node = env_data.worker_count;
    size_t w_idx, read_count = 0;
    char* affinity_copy = nullptr;
    char* affinity_to_parse = getenv(CCL_WORKER_AFFINITY);
    char* proc_id_str;
    char* tmp;
    size_t proc_count;

    env_data.worker_affinity.resize(workers_per_node);

    if (affinity_to_parse && strcmp(affinity_to_parse, "auto") == 0)
    {
        return ccl_env_parse_auto_worker_affinity();
    }

    if (!affinity_to_parse || strlen(affinity_to_parse) == 0)
    {
        /* generate default affinity */
        proc_count = sysconf(_SC_NPROCESSORS_ONLN);
        for (w_idx = 0; w_idx < workers_per_node; w_idx++)
        {
            if (w_idx < proc_count)
            {
                env_data.worker_affinity[w_idx] = proc_count - w_idx - 1;
            }
            else
            {
                env_data.worker_affinity[w_idx] = env_data.worker_affinity[w_idx % proc_count];
            }
        }
        read_env = 1;
        CCL_FREE(affinity_copy);
        return read_env;
    }

    /* create copy of original buffer cause it will be modified in strsep */
    size_t affinity_len = strlen(affinity_to_parse);
    affinity_copy = static_cast<char*>(CCL_MALLOC(affinity_len, "affinity_copy"));
    memcpy(affinity_copy, affinity_to_parse, affinity_len);
    tmp = affinity_copy;

    for (w_idx = 0; w_idx < workers_per_node; w_idx++)
    {
        proc_id_str = strsep(&tmp, ",");
        if (proc_id_str != NULL)
        {
            if (atoi(proc_id_str) < 0)
            {
                LOG_ERROR("unexpected proc_id ", proc_id_str, ", affinity string ", affinity_to_parse);
                read_env = 0;
                CCL_FREE(affinity_copy);
                return read_env;
            }
            env_data.worker_affinity[w_idx] = std::strtoul(proc_id_str, nullptr, 10);
            read_count++;
        }
        else
        {
            LOG_ERROR("unexpected end of affinity string, expected ", workers_per_node, " numbers, read ", read_count,
                      ", affinity string ", affinity_to_parse);
            read_env = 0;
            CCL_FREE(affinity_copy);
            return read_env;
        }
    }
    if (read_count < workers_per_node)
    {
        LOG_ERROR(
            "unexpected number of processors (specify 1 logical processor per 1 progress thread), affinity string ",
            affinity_to_parse);
        read_env = 0;
        CCL_FREE(affinity_copy);
        return read_env;
    }
    read_env = 1;

    CCL_FREE(affinity_copy);
    return read_env;
}

int ccl_env_parse_allgatherv_algo()
{
    char* env = getenv(CCL_ALLGATHERV);
    if (env)
    {
        if (strcmp(env, "naive") == 0)
            env_data.allgatherv_algo = ccl_allgatherv_algo_naive;
        else if (strcmp(env, "flat") == 0)
            env_data.allgatherv_algo = ccl_allgatherv_algo_flat;
        else if (strcmp(env, "multi_bcast") == 0)
            env_data.allgatherv_algo = ccl_allgatherv_algo_multi_bcast;
        else if (strcmp(env, "direct") == 0)
            env_data.allgatherv_algo = ccl_allgatherv_algo_direct;
        else
        {
            CCL_THROW("unknown ", CCL_ALLGATHERV, " ", env);
            return 0;
        }
    }
    return 1;
}

int ccl_env_parse_allreduce_algo()
{
    char* env = getenv(CCL_ALLREDUCE);
    if (env)
    {
        if (strcmp(env, "tree") == 0)
            env_data.allreduce_algo = ccl_allreduce_algo_rabenseifner;
        else if (strcmp(env, "starlike") == 0)
            env_data.allreduce_algo = ccl_allreduce_algo_starlike;
        else if (strcmp(env, "ring") == 0)
            env_data.allreduce_algo = ccl_allreduce_algo_ring;
        else if (strcmp(env, "ring_rma") == 0)
            env_data.allreduce_algo = ccl_allreduce_algo_ring_rma;
        else if (strcmp(env, "double_tree") == 0)
            env_data.allreduce_algo = ccl_allreduce_algo_double_tree;
        else if (strcmp(env, "direct") == 0)
            env_data.allreduce_algo = ccl_allreduce_algo_direct;
        else
        {
            CCL_THROW("unknown ", CCL_ALLREDUCE, " ", env);
            return 0;
        }
    }
    return 1;
}

int ccl_env_parse_barrier_algo()
{
    char* env = getenv(CCL_BARRIER);
    if (env)
    {
        if (strcmp(env, "ring") == 0)
            env_data.barrier_algo = ccl_barrier_algo_ring;
        else if (strcmp(env, "direct") == 0)
            env_data.barrier_algo = ccl_barrier_algo_direct;
        else
        {
            CCL_THROW("unknown ", CCL_BARRIER, " ", env);
            return 0;
        }
    }
    return 1;
}

int ccl_env_parse_bcast_algo()
{
    char* env = getenv(CCL_BCAST);
    if (env)
    {
        if (strcmp(env, "ring") == 0)
            env_data.bcast_algo = ccl_bcast_algo_ring;
        else if (strcmp(env, "double_tree") == 0)
            env_data.bcast_algo = ccl_bcast_algo_double_tree;
        else if (strcmp(env, "naive") == 0)
            env_data.bcast_algo = ccl_bcast_algo_naive;
        else if (strcmp(env, "direct") == 0)
            env_data.bcast_algo = ccl_bcast_algo_direct;
        else
        {
            CCL_THROW("unknown ", CCL_BCAST, " ", env);
            return 0;
        }
    }
    return 1;
}

int ccl_env_parse_reduce_algo()
{
    char* env = getenv(CCL_REDUCE);
    if (env)
    {
        if (strcmp(env, "tree") == 0)
            env_data.reduce_algo = ccl_reduce_algo_tree;
        else if (strcmp(env, "double_tree") == 0)
            env_data.reduce_algo = ccl_reduce_algo_double_tree;
        else if (strcmp(env, "direct") == 0)
            env_data.reduce_algo = ccl_reduce_algo_direct;
        else
        {
            CCL_THROW("unknown ", CCL_REDUCE, " ", env);
            return 0;
        }
    }
    return 1;
}

int ccl_env_parse_sparse_allreduce_algo()
{
    char* env = getenv(CCL_SPARSE_ALLREDUCE);
    if (env)
    {
        if (strcmp(env, "basic") == 0)
            env_data.sparse_allreduce_algo = ccl_sparse_allreduce_algo_basic;
        else if (strcmp(env, "mask") == 0)
            env_data.sparse_allreduce_algo = ccl_sparse_allreduce_algo_mask;
        else
        {
            CCL_THROW("unknown ", CCL_SPARSE_ALLREDUCE, " ", env);
            return 0;
        }
    }
    return 1;
}

int ccl_env_parse_priority_mode()
{
    char* env = getenv(CCL_PRIORITY);
    if (env)
    {
        if (strcmp(env, "none") == 0)
            env_data.priority_mode = ccl_priority_none;
        else if (strcmp(env, "direct") == 0)
            env_data.priority_mode = ccl_priority_direct;
        else if (strcmp(env, "lifo") == 0)
            env_data.priority_mode = ccl_priority_lifo;
        else
            CCL_FATAL("unexpected priority_mode ", env);
    }
    return 1;
}

int ccl_env_parse_yield_type()
{
    char* env = getenv(CCL_YIELD);
    if (env)
    {
        if (strcmp(env, "none") == 0)
            env_data.yield_type = ccl_yield_none;
        else if (strcmp(env, "pause") == 0)
            env_data.yield_type = ccl_yield_pause;
        else if (strcmp(env, "sleep") == 0)
            env_data.yield_type = ccl_yield_sleep;
        else if (strcmp(env, "sched_yield") == 0)
            env_data.yield_type = ccl_yield_sched_yield;
        else
        {
            CCL_THROW("unknown ", CCL_YIELD, " ", env);
            return 0;
        }
    }
    return 1; 
}

int ccl_env_parse_atl_transport()
{
    char* atl_transport = getenv(CCL_ATL_TRANSPORT);
    if (atl_transport)
    {
        if (strcmp(atl_transport, "OFI") == 0)
        {
            env_data.allgatherv_algo = ccl_allgatherv_algo_naive;
            env_data.allreduce_algo = ccl_allreduce_algo_rabenseifner;
            env_data.barrier_algo = ccl_barrier_algo_ring;
            env_data.bcast_algo = ccl_bcast_algo_ring;
            env_data.reduce_algo = ccl_reduce_algo_tree;
            env_data.sparse_allreduce_algo = ccl_sparse_allreduce_algo_basic;
        }
    }
    return 0;
}

int ccl_env_parse_cache_key()
{
    char* env = getenv(CCL_CACHE_KEY);
    if (env)
    {
        if (strcmp(env, "full") == 0)
            env_data.cache_key = ccl_cache_key_full;
        else if (strcmp(env, "match_id") == 0)
            env_data.cache_key = ccl_cache_key_match_id;
        else
        {
            CCL_THROW("unknown ", CCL_CACHE_KEY, " ", env);
            return 0;
        }
    }
    return 1; 
}

const char* ccl_allgatherv_algo_to_str(ccl_allgatherv_algo algo)
{
    switch (algo)
    {
        case ccl_allgatherv_algo_naive:
            return "naive";
        case ccl_allgatherv_algo_flat:
            return "flat";
        case ccl_allgatherv_algo_multi_bcast:
            return "multi_bcast";
        case ccl_allgatherv_algo_direct:
            return "direct";
        default:
            CCL_FATAL("unknown allgatherv_algo ", algo);
    }
}

const char* ccl_allreduce_algo_to_str(ccl_allreduce_algo algo)
{
    switch (algo)
    {
        case ccl_allreduce_algo_rabenseifner:
            return "tree";
        case ccl_allreduce_algo_starlike:
            return "starlike";
        case ccl_allreduce_algo_ring:
            return "ring";
        case ccl_allreduce_algo_ring_rma:
            return "ring_rma";
        case ccl_allreduce_algo_double_tree:
            return "double_tree";
        case ccl_allreduce_algo_direct:
            return "direct";
        default:
            CCL_FATAL("unknown allreduce_algo ", algo);
    }
}

const char* ccl_barrier_algo_to_str(ccl_barrier_algo algo)
{
    switch (algo)
    {
        case ccl_barrier_algo_ring:
            return "ring";
        case ccl_barrier_algo_direct:
            return "direct";
        default:
            CCL_FATAL("unknown barrier_algo ", algo);
    }
}

const char* ccl_bcast_algo_to_str(ccl_bcast_algo algo)
{
    switch (algo)
    {
        case ccl_bcast_algo_ring:
            return "ring";
        case ccl_bcast_algo_double_tree:
            return "double_tree";
        case ccl_bcast_algo_naive:
            return "naive";
        case ccl_bcast_algo_direct:
            return "direct";
        default:
            CCL_FATAL("unknown bcast_algo ", algo);
    }
}

const char* ccl_reduce_algo_to_str(ccl_reduce_algo algo)
{
    switch (algo)
    {
        case ccl_reduce_algo_tree:
            return "tree";
        case ccl_reduce_algo_double_tree:
            return "double_tree";
        case ccl_reduce_algo_direct:
            return "direct";
        default:
            CCL_FATAL("unknown reduce_algo ", algo);
    }
}

const char* ccl_sparse_allreduce_algo_to_str(ccl_sparse_allreduce_algo algo)
{
    switch (algo)
    {
        case ccl_sparse_allreduce_algo_basic:
            return "basic";
        case ccl_sparse_allreduce_algo_mask:
            return "mask";
        default:
            CCL_FATAL("unknown sparse_alleduce_algo ", algo);
    }
}

const char* ccl_priority_mode_to_str(ccl_priority_mode mode)
{
    switch (mode)
    {
        case ccl_priority_none:
            return "none";
        case ccl_priority_direct:
            return "direct";
        case ccl_priority_lifo:
            return "lifo";
        default:
            CCL_FATAL("unknown priority_mode ", mode);
    }
}

const char* ccl_yield_type_to_str(ccl_yield_type type)
{
    switch (type)
    {
        case ccl_yield_none:
            return "none";
        case ccl_yield_pause:
            return "pause";
        case ccl_yield_sleep:
            return "sleep";
        case ccl_yield_sched_yield:
            return "sched_yield";
        default:
            CCL_FATAL("unknown yield_type ", type);
    }
}

const char* ccl_cache_key_to_str(ccl_cache_key key)
{
    switch (key)
    {
        case ccl_cache_key_full:
            return "full";
        case ccl_cache_key_match_id:
            return "match_id";
        default:
            CCL_FATAL("unknown cache_key ", key);
    }
}
