#pragma once

#include <immintrin.h>
#include <sched.h>
#include <time.h>

#include "common/env/env.hpp"
#include "common/log/log.hpp"
#include "common/utils/yield.hpp"

enum ccl_yield_type
{
    ccl_yield_none = 0,
    ccl_yield_pause = 1,
    ccl_yield_sleep = 2,
    ccl_yield_sched_yield = 3,

    ccl_yield_last_value
};

inline void ccl_yield(ccl_yield_type yield_type)
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

inline const char* ccl_yield_type_to_str(ccl_yield_type type)
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
            CCL_FATAL("unexpected yield_type ", type);
    }
}
