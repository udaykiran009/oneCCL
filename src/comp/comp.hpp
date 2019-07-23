#pragma once

#include "common/datatype/datatype.hpp"
#include "ccl_types.h"

ccl_status_t ccl_comp_copy(const void *in_buf, void *out_buf, size_t count, ccl_datatype_internal_t dtype);
ccl_status_t ccl_comp_reduce(const void *in_buf, size_t in_count, void *inout_buf, size_t *out_count,
                             ccl_datatype_internal_t dtype, ccl_reduction_t reduction, ccl_reduction_fn_t reduction_fn);
const char *ccl_reduction_to_str(ccl_reduction_t type);
