#pragma once

#include <immintrin.h>
#include <sched.h>
#include <time.h>

#include "common/env/env.hpp"

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
