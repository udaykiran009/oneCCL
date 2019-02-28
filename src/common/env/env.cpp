#include "common/env/env.hpp"
#include "common/log/log.hpp"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

mlsl_env_data env_data =
{
    .log_level = ERROR,
    .sched_dump = 0,
    .worker_count = 1,
    .worker_offload = 1,
    .out_of_order_support = 0,
    .worker_affinity = NULL,
    .priority_mode = mlsl_priority_none,
    .allreduce_algo = mlsl_allreduce_algo_rabenseifner,
    .sparse_allreduce_algo = mlsl_sparse_allreduce_algo_basic,
    .enable_rma = 0,
    .enable_fusion = 0,
    .fusion_bytes_threshold = 16384,
    .fusion_count_threshold = 256,
    .fusion_check_urgent = 1,
    .fusion_cycle_ms = 0.2
};

const char *mlsl_priority_mode_to_str(mlsl_priority_mode mode)
{
    switch (mode) {
        case mlsl_priority_none:
            return "NONE";
        case mlsl_priority_direct:
            return "DIRECT";
        case mlsl_priority_lifo:
            return "LIFO";
        default:
            MLSL_ASSERT_FMT(0, "unexpected prio_mode %d", mode);
            return "(out of range)";
    }
}

const char *mlsl_allreduce_algo_to_str(mlsl_allreduce_algo algo)
{
    switch (algo) {
        case mlsl_allreduce_algo_rabenseifner:
            return "tree";
        case mlsl_allreduce_algo_starlike:
            return "starlike";
        case mlsl_allreduce_algo_ring:
            return "ring";
        case mlsl_allreduce_algo_ring_rma:
            return "ring_rma";
        default:
            MLSL_ASSERT_FMT(0, "unexpected allreduce_algo %d", algo);
            return "(out of range)";
    }
}

void mlsl_env_parse()
{
    mlsl_env_2_int("MLSL_LOG_LEVEL", &env_data.log_level);
    mlsl_env_2_int("MLSL_SCHED_DUMP", &env_data.sched_dump);
    mlsl_env_2_int("MLSL_WORKER_COUNT", &env_data.worker_count);
    mlsl_env_2_int("MLSL_WORKER_OFFLOAD", &env_data.worker_offload);
    mlsl_env_2_int("MLSL_OUT_OF_ORDER_SUPPORT", &env_data.out_of_order_support);
    mlsl_env_2_int("MLSL_ENABLE_RMA", &env_data.enable_rma);
    mlsl_env_2_int("MLSL_ENABLE_FUSION", &env_data.enable_fusion);
    mlsl_env_2_int("MLSL_FUSION_BYTES_THRESHOLD", &env_data.fusion_bytes_threshold);
    mlsl_env_2_int("MLSL_FUSION_COUNT_THRESHOLD", &env_data.fusion_count_threshold);
    mlsl_env_2_int("MLSL_FUSION_CHECK_URGENT", &env_data.fusion_check_urgent);
    mlsl_env_2_float("MLSL_FUSION_CYCLE_MS", &env_data.fusion_cycle_ms);
    mlsl_env_parse_priority_mode();
    mlsl_env_parse_affinity();
    mlsl_env_parse_allreduce_algo();

    MLSL_ASSERTP(env_data.worker_count >= 1);
}

void mlsl_env_free()
{
    MLSL_FREE(env_data.worker_affinity);
}

void mlsl_env_print()
{
    MLSL_LOG(INFO, "MLSL_LOG_LEVEL: %d", env_data.log_level);
    MLSL_LOG(INFO, "MLSL_SCHED_DUMP: %d", env_data.sched_dump);
    MLSL_LOG(INFO, "MLSL_WORKER_COUNT: %d", env_data.worker_count);
    MLSL_LOG(INFO, "MLSL_WORKER_OFFLOAD: %d", env_data.worker_offload);
    MLSL_LOG(INFO, "MLSL_OUT_OF_ORDER_SUPPORT: %d", env_data.out_of_order_support);
    MLSL_LOG(INFO, "MLSL_ENABLE_RMA: %d", env_data.enable_rma);
    MLSL_LOG(INFO, "MLSL_ENABLE_FUSION: %d", env_data.enable_fusion);
    MLSL_LOG(INFO, "MLSL_FUSION_BYTES_THRESHOLD: %d", env_data.fusion_bytes_threshold);
    MLSL_LOG(INFO, "MLSL_FUSION_COUNT_THRESHOLD: %d", env_data.fusion_count_threshold);
    MLSL_LOG(INFO, "MLSL_FUSION_CHECK_URGENT: %d", env_data.fusion_check_urgent);
    MLSL_LOG(INFO, "MLSL_FUSION_CYCLE_MS: %.1f", env_data.fusion_cycle_ms);
    MLSL_LOG(INFO, "MLSL_PRIORITY_MODE: %s", mlsl_priority_mode_to_str(env_data.priority_mode));
    MLSL_LOG(INFO, "MLSL_ALLREDUCE_ALGO: %s", mlsl_allreduce_algo_to_str(env_data.allreduce_algo));
    mlsl_env_print_affinity();
}

int mlsl_env_2_int(const char* env_name, int* val)
{
    const char* val_ptr;
    val_ptr = getenv(env_name);
    if (val_ptr)
    {
        *val = std::strtol(val_ptr, NULL, 10);
        return 1;
    }
    return 0;
}

int mlsl_env_2_float(const char* env_name, float* val)
{
    const char* val_ptr;
    val_ptr = getenv(env_name);
    if (val_ptr)
    {
        *val = std::strtof(val_ptr, NULL);
        return 1;
    }
    return 0;
}

int mlsl_env_parse_priority_mode()
{
    char *mode_env = getenv("MLSL_PRIORITY_MODE");
    if (mode_env)
    {
        if (strcmp(mode_env, "none") == 0)
            env_data.priority_mode = mlsl_priority_none;
        else if (strcmp(mode_env, "direct") == 0)
            env_data.priority_mode = mlsl_priority_direct;
        else if (strcmp(mode_env, "lifo") == 0)
            env_data.priority_mode = mlsl_priority_lifo;
        else
        {
            MLSL_ASSERT_FMT(0, "unexpected priority_mode %s", mode_env);
            return 0;
        }
    }
    return 1;
}

int mlsl_env_parse_affinity()
{
    int read_env = 0;
    size_t workers_per_node = env_data.worker_count;
    size_t w_idx, read_count = 0;
    char *affinity_copy = NULL, *affinity_to_parse = getenv("MLSL_WORKER_AFFINITY");
    char *proc_id_str, *tmp;
    size_t proc_count;

    env_data.worker_affinity = static_cast<int*>(MLSL_CALLOC(workers_per_node * sizeof(int), "worker_affinity"));

    if (!affinity_to_parse || strlen(affinity_to_parse) == 0) {
        /* generate default affinity */
        proc_count = sysconf(_SC_NPROCESSORS_ONLN);
        for (w_idx = 0; w_idx < workers_per_node; w_idx++) {
            if (w_idx < proc_count)
                env_data.worker_affinity[w_idx] = proc_count - w_idx - 1;
            else
                env_data.worker_affinity[w_idx] = env_data.worker_affinity[w_idx % proc_count];
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

    for (w_idx = 0; w_idx < workers_per_node; w_idx++) {
        proc_id_str = strsep(&tmp, ",");
        if (proc_id_str != NULL) {
            if (atoi(proc_id_str) < 0) {
                MLSL_LOG(ERROR, "unexpected proc_id %s, affinity string %s", proc_id_str, affinity_to_parse);
                read_env = 0;
                MLSL_FREE(affinity_copy);
                return read_env;
            }
            env_data.worker_affinity[w_idx] = atoi(proc_id_str);
            read_count++;
        } else {
            MLSL_LOG(ERROR, "unexpected end of affinity string, expected %zu numbers, read %zu, affinity string %s",
                     workers_per_node, read_count, affinity_to_parse);
            read_env = 0;
            MLSL_FREE(affinity_copy);
            return read_env;
        }
    }
    if (read_count < workers_per_node) {
        MLSL_LOG(ERROR, "unexpected number of processors (specify 1 logical processor per 1 progress thread), affinity string %s",
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
        MLSL_LOG(INFO, "worker: %zu, processor: %d",
                 w_idx, env_data.worker_affinity[w_idx]);
    return 1;
}

int mlsl_env_parse_allreduce_algo()
{
    char *mode_env = getenv("MLSL_ALLREDUCE_ALGO");
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
        else
        {
            MLSL_ASSERTP_FMT(0, "unexpected allreduce_algo %s", mode_env);
            return 0;
        }
    }
    return 1;
}
