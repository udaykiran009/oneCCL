#pragma once

#include "ccl_types.h"

typedef struct
{
    ccl_datatype_t type;
    size_t size;
    const char* name;
}
ccl_datatype_internal;
typedef const ccl_datatype_internal* ccl_datatype_internal_t;

ccl_status_t ccl_datatype_init();
size_t ccl_datatype_get_size(ccl_datatype_internal_t dtype);
const char* ccl_datatype_get_name(ccl_datatype_internal_t dtype);
ccl_datatype_internal_t ccl_datatype_get(ccl_datatype_t type);

extern ccl_datatype_internal_t ccl_dtype_internal_none;
extern ccl_datatype_internal_t ccl_dtype_internal_char;
extern ccl_datatype_internal_t ccl_dtype_internal_int;
extern ccl_datatype_internal_t ccl_dtype_internal_bfp16;
extern ccl_datatype_internal_t ccl_dtype_internal_float;
extern ccl_datatype_internal_t ccl_dtype_internal_double;
extern ccl_datatype_internal_t ccl_dtype_internal_int64;
extern ccl_datatype_internal_t ccl_dtype_internal_uint64;
