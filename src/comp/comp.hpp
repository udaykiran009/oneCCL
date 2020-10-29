#pragma once

#include "common/datatype/datatype.hpp"
#include "oneapi/ccl/ccl_types.hpp"
#include "internal_types.hpp"

ccl::status ccl_comp_copy(const void* in_buf,
                           void* out_buf,
                           size_t count,
                           const ccl_datatype& dtype);
ccl::status ccl_comp_reduce(const void* in_buf,
                             size_t in_count,
                             void* inout_buf,
                             size_t* out_count,
                             const ccl_datatype& dtype,
                             ccl::reduction reduction,
                             ccl::reduction_fn reduction_fn,
                             const ccl::fn_context* context = nullptr);
ccl::status ccl_comp_batch_reduce(const void* in_buf,
                                   const std::vector<size_t>& offsets,
                                   size_t in_count,
                                   void* inout_buf,
                                   size_t* out_count,
                                   const ccl_datatype& dtype,
                                   ccl::reduction reduction,
                                   ccl::reduction_fn reduction_fn,
                                   const ccl::fn_context* context,
                                   int bf16_keep_precision_mode,
                                   float* tmp,
                                   float* acc);
const char* ccl_reduction_to_str(ccl::reduction type);
