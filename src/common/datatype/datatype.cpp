#include "common/datatype/datatype.hpp"
#include "common/log/log.hpp"

iccl_datatype_internal_t iccl_dtype_internal_none;
iccl_datatype_internal_t iccl_dtype_internal_char;
iccl_datatype_internal_t iccl_dtype_internal_int;
iccl_datatype_internal_t iccl_dtype_internal_bfp16;
iccl_datatype_internal_t iccl_dtype_internal_float;
iccl_datatype_internal_t iccl_dtype_internal_double;
iccl_datatype_internal_t iccl_dtype_internal_int64;
iccl_datatype_internal_t iccl_dtype_internal_uint64;

const iccl_datatype_internal iccl_dtype_internal_none_value = { .type = iccl_dtype_char, .size = 0, .name = "NONE" };
const iccl_datatype_internal iccl_dtype_internal_char_value = { .type = iccl_dtype_char, .size = 1, .name = "CHAR" };
const iccl_datatype_internal iccl_dtype_internal_int_value = { .type = iccl_dtype_int, .size = 4, .name = "INT" };
const iccl_datatype_internal iccl_dtype_internal_bfp16_value = { .type = iccl_dtype_bfp16, .size = 2, .name = "BFP16" };
const iccl_datatype_internal iccl_dtype_internal_float_value = { .type = iccl_dtype_float, .size = 4, .name = "FLOAT" };
const iccl_datatype_internal iccl_dtype_internal_double_value = { .type = iccl_dtype_double, .size = 8, .name = "DOUBLE" };
const iccl_datatype_internal iccl_dtype_internal_int64_value = { .type = iccl_dtype_int64, .size = 8, .name = "INT64" };
const iccl_datatype_internal iccl_dtype_internal_uint64_value = { .type = iccl_dtype_uint64, .size = 8, .name = "UINT64" };

iccl_status_t iccl_datatype_init()
{
    iccl_dtype_internal_none = &iccl_dtype_internal_none_value;
    iccl_dtype_internal_char = &iccl_dtype_internal_char_value;
    iccl_dtype_internal_int = &iccl_dtype_internal_int_value;
    iccl_dtype_internal_bfp16 = &iccl_dtype_internal_bfp16_value;
    iccl_dtype_internal_float = &iccl_dtype_internal_float_value;
    iccl_dtype_internal_double = &iccl_dtype_internal_double_value;
    iccl_dtype_internal_int64 = &iccl_dtype_internal_int64_value;
    iccl_dtype_internal_uint64 = &iccl_dtype_internal_uint64_value;
    return iccl_status_success;
}

size_t iccl_datatype_get_size(iccl_datatype_internal_t dtype)
{
    ICCL_THROW_IF_NOT(dtype, "empty dtype");
    ICCL_ASSERT(dtype->size > 0);
    return dtype->size;
}

const char* iccl_datatype_get_name(iccl_datatype_internal_t dtype)
{
    ICCL_ASSERT(dtype);
    return dtype->name;
}

iccl_datatype_internal_t iccl_datatype_get(iccl_datatype_t type)
{
    iccl_datatype_internal_t dtype = NULL;
    switch (type)
    {
        case iccl_dtype_char: { dtype = iccl_dtype_internal_char; break; }
        case iccl_dtype_int: { dtype = iccl_dtype_internal_int; break; }
        case iccl_dtype_bfp16: { dtype = iccl_dtype_internal_bfp16; break; }
        case iccl_dtype_float: { dtype = iccl_dtype_internal_float; break; }
        case iccl_dtype_double: { dtype = iccl_dtype_internal_double; break; }
        case iccl_dtype_int64: { dtype = iccl_dtype_internal_int64; break; }
        case iccl_dtype_uint64: { dtype = iccl_dtype_internal_uint64; break; }
        default: ICCL_FATAL("unexpected dtype ", type);
    }
    return dtype;
}
