#include "env.h"
#include "log.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

mlsl_env_data env_data = {
                           .log_level = ERROR,
                           .dump_sched = 0,
                           .worker_count = 1,
                           .worker_affinity = NULL
                         };

void mlsl_env_parse()
{
    mlsl_env_2_int("MLSL_LOG_LEVEL", (int*)&env_data.log_level);
    mlsl_env_2_int("MLSL_DUMP_SCHED", (int*)&env_data.dump_sched);
    mlsl_env_2_int("MLSL_WORKER_COUNT", (int*)&env_data.worker_count);
    mlsl_env_parse_affinity();
}

void mlsl_env_free()
{
    MLSL_FREE(env_data.worker_affinity);
}

void mlsl_env_print()
{
    MLSL_LOG(INFO, "MLSL_LOG_LEVEL: %d", env_data.log_level);
    MLSL_LOG(INFO, "MLSL_DUMP_SCHED: %d", env_data.dump_sched);
    MLSL_LOG(INFO, "MLSL_WORKER_COUNT: %d", env_data.worker_count);
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

int mlsl_env_parse_affinity()
{
    size_t workers_per_node = env_data.worker_count;
    size_t w_idx, read_count = 0;
    char *affinity_copy = NULL, *affinity_to_parse = getenv("MLSL_WORKER_AFFINITY");
    char *proc_id_str, *tmp;
    size_t proc_count;

    env_data.worker_affinity = MLSL_CALLOC(workers_per_node * sizeof(int), "worker_affinity");

    if (!affinity_to_parse || strlen(affinity_to_parse) == 0) {
        /* generate default affinity */
        proc_count = sysconf(_SC_NPROCESSORS_ONLN);
        for (w_idx = 0; w_idx < workers_per_node; w_idx++) {
            if (w_idx < proc_count)
                env_data.worker_affinity[w_idx] = proc_count - w_idx - 1;
            else
                env_data.worker_affinity[w_idx] = env_data.worker_affinity[w_idx % proc_count];
        }
        return 1;
    }

    /* create copy of original buffer cause it will be modified in strsep */
    affinity_copy = strdup(affinity_to_parse);
    MLSL_ASSERTP(affinity_copy);
    tmp = affinity_copy;

    for (w_idx = 0; w_idx < workers_per_node; w_idx++) {
        proc_id_str = strsep(&tmp, " \n\t,");
        if (proc_id_str != NULL) {
            if (atoi(proc_id_str) < 0) {
                MLSL_LOG(ERROR, "unexpected proc_id %s, affinity string %s", proc_id_str, affinity_to_parse);
                return 0;
            }
            env_data.worker_affinity[w_idx] = atoi(proc_id_str);
            read_count++;
        } else {
            MLSL_LOG(ERROR, "unexpected end of affinity string, expected %zu numbers, read %zu, affinity string %s",
                     workers_per_node, read_count, affinity_to_parse);
            return 0;
        }
    }
    if (read_count < workers_per_node) {
        MLSL_LOG(ERROR, "unexpected number of processors (specify 1 logical processor per 1 progress thread), affinity string %s",
                 affinity_to_parse);
        return 0;
    }

    MLSL_FREE(affinity_copy);
    return 1;
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
