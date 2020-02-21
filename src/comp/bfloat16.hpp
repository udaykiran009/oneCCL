#pragma once

#include "ccl_types.h"

void ccl_bf16_reduce(const void* in_buf, size_t in_cnt,
                     void* inout_buf, size_t* out_cnt,
                     ccl_reduction_t reduction_op);
