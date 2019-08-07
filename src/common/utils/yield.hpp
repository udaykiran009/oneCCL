#pragma once

#include <immintrin.h>
#include <sched.h>
#include <time.h>

enum ccl_yield_type
{
    ccl_yield_none,
    ccl_yield_pause,
    ccl_yield_sleep,
    ccl_yield_sched_yield,

    ccl_yield_last_value
};

void ccl_yield(ccl_yield_type yield_type);
const char* ccl_yield_type_to_str(ccl_yield_type type);
