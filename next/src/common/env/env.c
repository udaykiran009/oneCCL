#include "env.h"
#include "log.h"

mlsl_env_data env_data = {
                           .log_level = ERROR,
                           .dump_sched = 0
                         };

void mlsl_env_parse()
{
    mlsl_env_2_int("MLSL_LOG_LEVEL", (int*)&env_data.log_level);
    mlsl_env_2_int("MLSL_DUMP_SCHED", (int*)&env_data.dump_sched);
}

void mlsl_env_print()
{
    MLSL_LOG(INFO, "MLSL_LOG_LEVEL: %d", env_data.log_level);
    MLSL_LOG(INFO, "MLSL_DUMP_SCHED: %d", env_data.dump_sched);
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
