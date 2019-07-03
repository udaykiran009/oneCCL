#pragma once

#include "common/datatype/datatype.hpp"
#include "iccl_types.h"

iccl_status_t iccl_comp_copy(const void *in_buf, void *out_buf, size_t count, iccl_datatype_internal_t dtype);
iccl_status_t iccl_comp_reduce(const void *in_buf, size_t in_count, void *inout_buf, size_t *out_count,
                               iccl_datatype_internal_t dtype, iccl_reduction_t reduction, iccl_reduction_fn_t reduction_fn);
const char *iccl_reduction_to_str(iccl_reduction_t type);
