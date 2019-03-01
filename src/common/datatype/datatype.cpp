#include "common/datatype/datatype.hpp"
#include "common/log/log.hpp"

mlsl_datatype_internal_t mlsl_dtype_internal_none;
mlsl_datatype_internal_t mlsl_dtype_internal_char;
mlsl_datatype_internal_t mlsl_dtype_internal_int;
mlsl_datatype_internal_t mlsl_dtype_internal_bfp16;
mlsl_datatype_internal_t mlsl_dtype_internal_float;
mlsl_datatype_internal_t mlsl_dtype_internal_double;
mlsl_datatype_internal_t mlsl_dtype_internal_int64;
mlsl_datatype_internal_t mlsl_dtype_internal_uint64;

const mlsl_datatype_internal mlsl_dtype_internal_none_value = { .type = mlsl_dtype_char, .size = 0, .name = "NONE" };
const mlsl_datatype_internal mlsl_dtype_internal_char_value = { .type = mlsl_dtype_char, .size = 1, .name = "CHAR" };
const mlsl_datatype_internal mlsl_dtype_internal_int_value = { .type = mlsl_dtype_int, .size = 4, .name = "INT" };
const mlsl_datatype_internal mlsl_dtype_internal_bfp16_value = { .type = mlsl_dtype_bfp16, .size = 2, .name = "BFP16" };
const mlsl_datatype_internal mlsl_dtype_internal_float_value = { .type = mlsl_dtype_float, .size = 4, .name = "FLOAT" };
const mlsl_datatype_internal mlsl_dtype_internal_double_value = { .type = mlsl_dtype_double, .size = 8, .name = "DOUBLE" };
const mlsl_datatype_internal mlsl_dtype_internal_int64_value = { .type = mlsl_dtype_int64, .size = 8, .name = "INT64" };
const mlsl_datatype_internal mlsl_dtype_internal_uint64_value = { .type = mlsl_dtype_uint64, .size = 8, .name = "UINT64" };

mlsl_status_t mlsl_datatype_init()
{
    mlsl_dtype_internal_none = &mlsl_dtype_internal_none_value;
    mlsl_dtype_internal_char = &mlsl_dtype_internal_char_value;
    mlsl_dtype_internal_int = &mlsl_dtype_internal_int_value;
    mlsl_dtype_internal_bfp16 = &mlsl_dtype_internal_bfp16_value;
    mlsl_dtype_internal_float = &mlsl_dtype_internal_float_value;
    mlsl_dtype_internal_double = &mlsl_dtype_internal_double_value;
    mlsl_dtype_internal_int64 = &mlsl_dtype_internal_int64_value;
    mlsl_dtype_internal_uint64 = &mlsl_dtype_internal_uint64_value;
    return mlsl_status_success;
}

size_t mlsl_datatype_get_size(mlsl_datatype_internal_t dtype)
{
    MLSL_THROW_IF_NOT(dtype, "empty dtype");
    MLSL_ASSERT(dtype->size > 0);
    return dtype->size;
}

const char* mlsl_datatype_get_name(mlsl_datatype_internal_t dtype)
{
    MLSL_ASSERT(dtype);
    return dtype->name;
}

mlsl_datatype_internal_t mlsl_datatype_get(mlsl_datatype_t type)
{
    mlsl_datatype_internal_t dtype = NULL;
    switch (type)
    {
        case mlsl_dtype_char: { dtype = mlsl_dtype_internal_char; break; }
        case mlsl_dtype_int: { dtype = mlsl_dtype_internal_int; break; }
        case mlsl_dtype_bfp16: { dtype = mlsl_dtype_internal_bfp16; break; }
        case mlsl_dtype_float: { dtype = mlsl_dtype_internal_float; break; }
        case mlsl_dtype_double: { dtype = mlsl_dtype_internal_double; break; }
        case mlsl_dtype_int64: { dtype = mlsl_dtype_internal_int64; break; }
        case mlsl_dtype_uint64: { dtype = mlsl_dtype_internal_uint64; break; }
        default: MLSL_FATAL("unexpected dtype %d", type);
    }
    return dtype;
}
