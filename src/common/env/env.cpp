#include "common/env/env.hpp"
#include "common/log/log.hpp"

#include <unistd.h>

mlsl_env_data env_data =
    {
        .log_level = static_cast<int>(mlsl_log_level::ERROR),
        .sched_dump = 0,
        .worker_count = 1,
        .worker_offload = 1,
        .out_of_order_support = 0,
        .worker_affinity = std::vector<int>(),
        .priority_mode = mlsl_priority_none,
        .allreduce_algo = mlsl_allreduce_algo_rabenseifner,
        .allgatherv_algo = mlsl_allgatherv_algo_naive,
        .bcast_algo = mlsl_bcast_algo_ring,
        .reduce_algo = mlsl_reduce_algo_tree,
        .barrier_algo = mlsl_barrier_algo_ring,
        .sparse_allreduce_algo = mlsl_sparse_allreduce_algo_basic,
        .enable_rma = 0,
        .enable_fusion = 0,
        .fusion_bytes_threshold = 16384,
        .fusion_count_threshold = 256,
        .fusion_check_urgent = 1,
        .fusion_cycle_ms = 0.2,
        .vector_allgatherv = 0
    };

const char* mlsl_priority_mode_to_str(mlsl_priority_mode mode)
{
    switch (mode) {
        case mlsl_priority_none:
            return "none";
        case mlsl_priority_direct:
            return "direct";
        case mlsl_priority_lifo:
            return "lifo";
        default:
            MLSL_FATAL("unexpected prio_mode ", mode);
    }
}

const char *mlsl_allgatherv_algo_to_str(mlsl_allgatherv_algo algo)
{
    switch (algo) {
        case mlsl_allgatherv_algo_naive:
            return "naive";
        case mlsl_allgatherv_algo_direct:
            return "direct";
        default:
            MLSL_FATAL("unexpected allgatherv_algo ", algo);
    }
}

const char* mlsl_allreduce_algo_to_str(mlsl_allreduce_algo algo)
{
    switch (algo)
    {
        case mlsl_allreduce_algo_rabenseifner:
            return "tree";
        case mlsl_allreduce_algo_starlike:
            return "starlike";
        case mlsl_allreduce_algo_ring:
            return "ring";
        case mlsl_allreduce_algo_ring_rma:
            return "ring_rma";
        case mlsl_allreduce_algo_double_tree:
            return "double_tree";
        case mlsl_allreduce_algo_direct:
            return "direct";
        default:
            MLSL_FATAL("unexpected allreduce_algo ", algo);
    }
}

const char* mlsl_reduce_algo_to_str(mlsl_reduce_algo algo)
{
    switch (algo)
    {
        case mlsl_reduce_algo_tree:
            return "tree";
        case mlsl_reduce_algo_double_tree:
            return "double_tree";
        case mlsl_reduce_algo_direct:
            return "direct";
        default:
            MLSL_FATAL("unexpected allreduce_algo ", algo);
    }
}

const char* mlsl_bcast_algo_to_str(mlsl_bcast_algo algo)
{
    switch (algo)
    {
        case mlsl_bcast_algo_ring:
            return "ring";
        case mlsl_bcast_algo_double_tree:
            return "double_tree";
        case mlsl_bcast_algo_direct:
            return "direct";
        default:
            MLSL_FATAL("unexpected allreduce_algo ", algo);
    }
}

const char *mlsl_barrier_algo_to_str(mlsl_barrier_algo algo)
{
    switch (algo) {
        case mlsl_barrier_algo_ring:
            return "ring";
        case mlsl_barrier_algo_direct:
            return "direct";
        default:
            MLSL_FATAL("unexpected barrier_algo ", algo);
    }
}

void mlsl_env_parse()
{
    mlsl_env_2_int(MLSL_LOG_LEVEL, env_data.log_level);
    mlsl_logger::set_log_level(static_cast<mlsl_log_level>(env_data.log_level));
    mlsl_env_2_int(MLSL_SCHED_DUMP, env_data.sched_dump);
    mlsl_env_2_size_t(MLSL_WORKER_COUNT, env_data.worker_count);
    mlsl_env_2_int(MLSL_WORKER_OFFLOAD, env_data.worker_offload);
    mlsl_env_2_int(MLSL_OUT_OF_ORDER_SUPPORT, env_data.out_of_order_support);
    mlsl_env_2_int(MLSL_ENABLE_RMA, env_data.enable_rma);
    mlsl_env_2_int(MLSL_ENABLE_FUSION, env_data.enable_fusion);
    mlsl_env_2_int(MLSL_FUSION_BYTES_THRESHOLD, env_data.fusion_bytes_threshold);
    mlsl_env_2_int(MLSL_FUSION_COUNT_THRESHOLD, env_data.fusion_count_threshold);
    mlsl_env_2_int(MLSL_FUSION_CHECK_URGENT, env_data.fusion_check_urgent);
    mlsl_env_2_float(MLSL_FUSION_CYCLE_MS, env_data.fusion_cycle_ms);
    mlsl_env_2_int(MLSL_VECTOR_ALLGATHERV, env_data.vector_allgatherv);

    mlsl_env_parse_priority_mode();
    mlsl_env_parse_allreduce_algo();
    mlsl_env_parse_bcast_algo();
    mlsl_env_parse_reduce_algo();
    mlsl_env_parse_barrier_algo();

    auto result = mlsl_env_parse_affinity();
    MLSL_THROW_IF_NOT(result == 1, "failed to parse ", MLSL_WORKER_AFFINITY);

    MLSL_THROW_IF_NOT(env_data.worker_count >= 1, "incorrect %s %d", MLSL_WORKER_COUNT, env_data.worker_count);
    if (env_data.enable_fusion)
    {
        MLSL_THROW_IF_NOT(env_data.fusion_bytes_threshold >= 1, "incorrect ",
                          MLSL_FUSION_BYTES_THRESHOLD, " ", env_data.fusion_bytes_threshold);
        MLSL_THROW_IF_NOT(env_data.fusion_count_threshold >= 1, "incorrect ",
                          MLSL_FUSION_COUNT_THRESHOLD, " ", env_data.fusion_count_threshold);
    }
}

void mlsl_env_print()
{
    LOG_INFO(MLSL_LOG_LEVEL, ": ", env_data.log_level);
    LOG_INFO(MLSL_SCHED_DUMP, ": ", env_data.sched_dump);
    LOG_INFO(MLSL_WORKER_COUNT, ": ", env_data.worker_count);
    LOG_INFO(MLSL_WORKER_OFFLOAD, ": ", env_data.worker_offload);
    LOG_INFO(MLSL_OUT_OF_ORDER_SUPPORT, ": ", env_data.out_of_order_support);
    LOG_INFO(MLSL_ENABLE_RMA, ": ", env_data.enable_rma);
    LOG_INFO(MLSL_ENABLE_FUSION, ": ", env_data.enable_fusion);
    LOG_INFO(MLSL_FUSION_BYTES_THRESHOLD, ": ", env_data.fusion_bytes_threshold);
    LOG_INFO(MLSL_FUSION_COUNT_THRESHOLD, ": ", env_data.fusion_count_threshold);
    LOG_INFO(MLSL_FUSION_CHECK_URGENT, ": ", env_data.fusion_check_urgent);
    LOG_INFO(MLSL_FUSION_CYCLE_MS, ": ", env_data.fusion_cycle_ms);
    LOG_INFO(MLSL_PRIORITY_MODE, ": ", mlsl_priority_mode_to_str(env_data.priority_mode));
    LOG_INFO(MLSL_ALLREDUCE_ALGO, ": ", mlsl_allreduce_algo_to_str(env_data.allreduce_algo));
    LOG_INFO(MLSL_REDUCE_ALGO, ": ", mlsl_reduce_algo_to_str(env_data.reduce_algo));
    LOG_INFO(MLSL_BCAST_ALGO, ": ", mlsl_bcast_algo_to_str(env_data.bcast_algo));
    LOG_INFO(MLSL_BARRIER_ALGO, ": ",  mlsl_barrier_algo_to_str(env_data.barrier_algo));

    mlsl_env_print_affinity();
}

int mlsl_env_2_int(const char* env_name,
                   int& val)
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

int mlsl_env_2_size_t(const char* env_name,
                      size_t& val)
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

int mlsl_env_2_float(const char* env_name,
                     float& val)
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

int mlsl_env_parse_priority_mode()
{
    char* mode_env = getenv(MLSL_PRIORITY_MODE);
    if (mode_env)
    {
        if (strcmp(mode_env, "none") == 0)
            env_data.priority_mode = mlsl_priority_none;
        else if (strcmp(mode_env, "direct") == 0)
            env_data.priority_mode = mlsl_priority_direct;
        else if (strcmp(mode_env, "lifo") == 0)
            env_data.priority_mode = mlsl_priority_lifo;
        else
            MLSL_FATAL("unexpected priority_mode ", mode_env);
    }
    return 1;
}

constexpr const char* AVAILABLE_CORES_ENV = "I_MPI_PIN_INFO";

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

static int parse_auto_affinity()
{
    char* available_cores = std::getenv(AVAILABLE_CORES_ENV);
    MLSL_THROW_IF_NOT(available_cores && strlen(available_cores) != 0, "auto pinning requires ",
                      AVAILABLE_CORES_ENV, " env variable to be set");
    std::vector<int> cores;
    str_to_array(available_cores + 1, cores, ',');

    MLSL_THROW_IF_NOT(env_data.worker_count <= cores.size(), "count of workers ", env_data.worker_count,
                      " exceeds the number of available cores ", cores.size());

    size_t mlsl_cores_start = cores.size() - 1 - env_data.worker_count;

    std::stringstream str;
    mlsl_logger::format(str, "mlsl uses cores: ");
    for(size_t idx = 0; idx < env_data.worker_count; ++idx)
    {
        env_data.worker_affinity[idx] = cores[mlsl_cores_start + idx];
        mlsl_logger::format(str, env_data.worker_affinity[idx], " ");
    }
    LOG_INFO(str.str());

    return 1;
}

int mlsl_env_parse_affinity()
{
    int read_env = 0;
    size_t workers_per_node = env_data.worker_count;
    size_t w_idx, read_count = 0;
    char* affinity_copy = nullptr;
    char* affinity_to_parse = getenv(MLSL_WORKER_AFFINITY);
    char* proc_id_str;
    char* tmp;
    size_t proc_count;

    env_data.worker_affinity.reserve(workers_per_node);

    if (affinity_to_parse && strcmp(affinity_to_parse, "auto") == 0)
    {
        return parse_auto_affinity();
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
        MLSL_FREE(affinity_copy);
        return read_env;
    }

    /* create copy of original buffer cause it will be modified in strsep */
    size_t affinity_len = strlen(affinity_to_parse);
    affinity_copy = static_cast<char*>(MLSL_MALLOC(affinity_len, "affinity_copy"));
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
                MLSL_FREE(affinity_copy);
                return read_env;
            }
            env_data.worker_affinity[w_idx] = atoi(proc_id_str);
            read_count++;
        }
        else
        {
            LOG_ERROR("unexpected end of affinity string, expected ", workers_per_node, " numbers, read ", read_count,
                      ", affinity string ", affinity_to_parse);
            read_env = 0;
            MLSL_FREE(affinity_copy);
            return read_env;
        }
    }
    if (read_count < workers_per_node)
    {
        LOG_ERROR(
            "unexpected number of processors (specify 1 logical processor per 1 progress thread), affinity string ",
            affinity_to_parse);
        read_env = 0;
        MLSL_FREE(affinity_copy);
        return read_env;
    }
    read_env = 1;

    MLSL_FREE(affinity_copy);
    return read_env;
}

int mlsl_env_print_affinity()
{
    size_t w_idx;
    size_t workers_per_node = env_data.worker_count;
    for (w_idx = 0; w_idx < workers_per_node; w_idx++)
    {
        LOG_INFO("worker: ", w_idx, ", processor: ", env_data.worker_affinity[w_idx]);
    }
    return 1;
}

int mlsl_env_parse_allreduce_algo()
{
    char* mode_env = getenv(MLSL_ALLREDUCE_ALGO);
    if (mode_env)
    {
        if (strcmp(mode_env, "tree") == 0)
            env_data.allreduce_algo = mlsl_allreduce_algo_rabenseifner;
        else if (strcmp(mode_env, "starlike") == 0)
            env_data.allreduce_algo = mlsl_allreduce_algo_starlike;
        else if (strcmp(mode_env, "ring") == 0)
            env_data.allreduce_algo = mlsl_allreduce_algo_ring;
        else if (strcmp(mode_env, "ring_rma") == 0)
            env_data.allreduce_algo = mlsl_allreduce_algo_ring_rma;
        else if (strcmp(mode_env, "direct") == 0)
            env_data.allreduce_algo = mlsl_allreduce_algo_direct;
        else if (strcmp(mode_env, "double_tree") == 0)
            env_data.allreduce_algo = mlsl_allreduce_algo_double_tree;
        else
        {
            MLSL_THROW("incorrect ", MLSL_ALLREDUCE_ALGO, " ", mode_env);
            return 0;
        }
    }
    return 1;
}

int mlsl_env_parse_bcast_algo()
{
    char* mode_env = getenv(MLSL_BCAST_ALGO);
    if (mode_env)
    {
        if (strcmp(mode_env, "ring") == 0)
            env_data.bcast_algo = mlsl_bcast_algo_ring;
        else if (strcmp(mode_env, "double_tree") == 0)
            env_data.bcast_algo = mlsl_bcast_algo_double_tree;
        else if (strcmp(mode_env, "direct") == 0)
            env_data.bcast_algo = mlsl_bcast_algo_direct;
        else
        {
            MLSL_THROW("incorrect ", MLSL_BCAST_ALGO, " ", mode_env);
            return 0;
        }
    }
    return 1;
}


int mlsl_env_parse_reduce_algo()
{
    char* mode_env = getenv(MLSL_REDUCE_ALGO);
    if (mode_env)
    {
        if (strcmp(mode_env, "tree") == 0)
            env_data.reduce_algo = mlsl_reduce_algo_tree;
        else if (strcmp(mode_env, "double_tree") == 0)
            env_data.reduce_algo = mlsl_reduce_algo_double_tree;
        else if (strcmp(mode_env, "direct") == 0)
            env_data.reduce_algo = mlsl_reduce_algo_direct;
        else
        {
            MLSL_THROW("incorrect ", MLSL_REDUCE_ALGO, " ", mode_env);
            return 0;
        }
    }
    return 1;
}

int mlsl_env_parse_allgatherv_algo()
{
    char *mode_env = getenv("MLSL_ALLGATHERV_ALGO");
    if (mode_env)
    {
        if (strcmp(mode_env, "naive") == 0)
            env_data.allgatherv_algo = mlsl_allgatherv_algo_naive;
        else if (strcmp(mode_env, "direct") == 0)
            env_data.allgatherv_algo = mlsl_allgatherv_algo_direct;
        else
        {
            MLSL_THROW("incorrect MLSL_ALLGATHERV_ALGO ", mode_env);
            return 0;
        }
    }
    return 1;
}

int mlsl_env_parse_barrier_algo()
{
    char *mode_env = getenv("MLSL_BARRIER_ALGO");
    if (mode_env)
    {
        if (strcmp(mode_env, "ring") == 0)
            env_data.barrier_algo = mlsl_barrier_algo_ring;
        else if (strcmp(mode_env, "direct") == 0)
            env_data.barrier_algo = mlsl_barrier_algo_direct;
        else
        {
            MLSL_THROW("incorrect MLSL_BARRIER_ALGO ", mode_env);
            return 0;
        }
    }
    return 1;
}
