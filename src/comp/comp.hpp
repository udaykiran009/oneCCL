#pragma once

#include "common/datatype/datatype.hpp"
#include "ccl_types.h"

ccl_status_t ccl_comp_copy(const void* in_buf,
                           void* out_buf,
                           size_t count,
                           const ccl_datatype& dtype);
ccl_status_t ccl_comp_reduce(const void* in_buf,
                             size_t in_count,
                             void* inout_buf,
                             size_t* out_count,
                             const ccl_datatype& dtype,
                             ccl_reduction_t reduction,
                             ccl_reduction_fn_t reduction_fn,
                             const ccl_fn_context_t* context = nullptr);
ccl_status_t ccl_comp_batch_reduce(const void* in_buf,
                                   const std::vector<size_t>& offsets,
                                   size_t in_count,
                                   void* inout_buf,
                                   size_t* out_count,
                                   const ccl_datatype& dtype,
                                   ccl_reduction_t reduction,
                                   ccl_reduction_fn_t reduction_fn,
                                   const ccl_fn_context_t* context,
                                   int bfp16_keep_precision_mode,
                                   float* tmp,
                                   float* acc);
const char* ccl_reduction_to_str(ccl_reduction_t type);
