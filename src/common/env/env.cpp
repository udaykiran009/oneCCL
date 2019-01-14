#include "common/env/env.hpp"
#include "common/log/log.hpp"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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
            return "RSAG";
        case mlsl_allreduce_algo_starlike:
            return "STARLIKE";
        default:
            MLSL_ASSERT_FMT(0, "unexpected allreduce_algo %d", algo);
            return "(out of range)";
    }
}

mlsl_env_data env_data =
{
   .log_level = ERROR,
   .sched_dump = 0,
   .worker_count = 1,
   .worker_offload = 1,
   .out_of_order_support = 0,
   .worker_affinity = nullptr,
   .priority_mode = mlsl_priority_none,
   .allreduce_algo = mlsl_allreduce_algo_rabenseifner
};

void mlsl_env_parse()
{
    mlsl_env_2_int("MLSL_LOG_LEVEL", &env_data.log_level);
    mlsl_env_2_int("MLSL_SCHED_DUMP", &env_data.sched_dump);
    mlsl_env_2_int("MLSL_WORKER_COUNT", &env_data.worker_count);
    mlsl_env_2_int("MLSL_WORKER_OFFLOAD", &env_data.worker_offload);
    mlsl_env_2_int("MLSL_OUT_OF_ORDER_SUPPORT", &env_data.out_of_order_support);
    mlsl_env_parse_priority_mode();
    mlsl_env_parse_affinity();
    mlsl_env_parse_allreduce_algo();
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
        const char* p;
        int sign = 1, value = 0;
        p = val_ptr;
        while (*p && IS_SPACE(*p))
            p++;
        if (*p == '-')
        {
            p++;
            sign = -1;
        }
        if (*p == '+')
            p++;

        while (*p && isdigit(*p))
            value = 10 * value + (*p++ - '0');

        if (*p)
        {
            MLSL_LOG(ERROR, "invalid character %c in %s", *p, env_name);
            return -1;
        }
        *val = sign * value;
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
        if (strcmp(mode_env, "rsag") == 0) // reduce_scatter + allgather
            env_data.allreduce_algo = mlsl_allreduce_algo_rabenseifner;
        else if (strcmp(mode_env, "starlike") == 0)
            env_data.allreduce_algo = mlsl_allreduce_algo_starlike;
        else
        {
            MLSL_ASSERT_FMT(0, "unexpected allreduce_algo %s", mode_env);
            return 0;
        }
    }
    return 1;
}
