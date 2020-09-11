#pragma once

#include <map>

#include <immintrin.h>
#include <sched.h>
#include <time.h>

enum ccl_yield_type {
    ccl_yield_none,
    ccl_yield_pause,
    ccl_yield_sleep,
    ccl_yield_sched_yield,

    ccl_yield_last_value
};

void ccl_yield(ccl_yield_type yield_type);

extern std::map<ccl_yield_type, std::string> ccl_yield_type_names;
