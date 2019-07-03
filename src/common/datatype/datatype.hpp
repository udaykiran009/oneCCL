#pragma once

#include "iccl_types.h"

typedef struct
{
    iccl_datatype_t type;
    size_t size;
    const char* name;
}
iccl_datatype_internal;
typedef const iccl_datatype_internal* iccl_datatype_internal_t;

iccl_status_t iccl_datatype_init();
size_t iccl_datatype_get_size(iccl_datatype_internal_t dtype);
const char* iccl_datatype_get_name(iccl_datatype_internal_t dtype);
iccl_datatype_internal_t iccl_datatype_get(iccl_datatype_t type);

extern iccl_datatype_internal_t iccl_dtype_internal_none;
extern iccl_datatype_internal_t iccl_dtype_internal_char;
extern iccl_datatype_internal_t iccl_dtype_internal_int;
extern iccl_datatype_internal_t iccl_dtype_internal_bfp16;
extern iccl_datatype_internal_t iccl_dtype_internal_float;
extern iccl_datatype_internal_t iccl_dtype_internal_double;
extern iccl_datatype_internal_t iccl_dtype_internal_int64;
extern iccl_datatype_internal_t iccl_dtype_internal_uint64;
