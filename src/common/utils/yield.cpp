#include "common/log/log.hpp"
#include "common/utils/yield.hpp"

void ccl_yield(ccl_yield_type yield_type)
{
    struct timespec sleep_time;

    switch (yield_type)
    {
        case ccl_yield_none:
            break;
        case ccl_yield_pause:
            _mm_pause();
            break;
        case ccl_yield_sleep:
            sleep_time.tv_sec = 0;
            sleep_time.tv_nsec = 0;
            nanosleep(&sleep_time, nullptr);
            break;
        case ccl_yield_sched_yield:
            sched_yield();
            break;
        default:
            break;
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
    return "unknown";
}
