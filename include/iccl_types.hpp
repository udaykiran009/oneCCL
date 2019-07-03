#pragma once

#include "iccl_types.h"

#include <stdexcept>

namespace iccl
{

/**
 * Supported iccl reduction types
 */
enum class reduction
{
    sum = iccl_reduction_sum,
    prod = iccl_reduction_prod,
    min = iccl_reduction_min,
    max = iccl_reduction_max,
    custom = iccl_reduction_custom
};

/**
 * Supported iccl data types
 */
enum class data_type
{
    dtype_char = iccl_dtype_char,
    dtype_int = iccl_dtype_int,
    dtype_bfp16 = iccl_dtype_bfp16,
    dtype_float = iccl_dtype_float,
    dtype_double = iccl_dtype_double,
    dtype_int64 = iccl_dtype_int64,
    dtype_uint64 = iccl_dtype_uint64
};

/**
 * Exception type that may be thrown by iccl API
 */
class iccl_error : public std::runtime_error
{
public:
    explicit iccl_error(const std::string& message) : std::runtime_error(message)
    {}

    explicit iccl_error(const char* message) : std::runtime_error(message)
    {}
};

}
