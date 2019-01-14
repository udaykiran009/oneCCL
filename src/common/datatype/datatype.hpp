#pragma once

#include "mlsl_types.h"

#define MLSL_POSTPONED_DTYPE ((mlsl_datatype_internal_t)(0xFFFFFFFFFFFFFFFF))

typedef struct
{
    mlsl_datatype_t type;
    size_t size;
    const char* name;

} mlsl_datatype_internal;
typedef const mlsl_datatype_internal* mlsl_datatype_internal_t;

mlsl_status_t mlsl_datatype_init();
size_t mlsl_datatype_get_size(mlsl_datatype_internal_t dtype);
const char* mlsl_datatype_get_name(mlsl_datatype_internal_t dtype);
mlsl_datatype_internal_t mlsl_datatype_get(mlsl_datatype_t type);

extern mlsl_datatype_internal_t mlsl_dtype_internal_char;
extern mlsl_datatype_internal_t mlsl_dtype_internal_int;
extern mlsl_datatype_internal_t mlsl_dtype_internal_bfp16;
extern mlsl_datatype_internal_t mlsl_dtype_internal_float;
extern mlsl_datatype_internal_t mlsl_dtype_internal_double;
extern mlsl_datatype_internal_t mlsl_dtype_internal_int64;
extern mlsl_datatype_internal_t mlsl_dtype_internal_uint64;
