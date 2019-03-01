#pragma once

#include "mlsl_types.h"

#include <stdexcept>

namespace mlsl
{

/**
 * Supported mlsl reduction types
 */
enum class reduction
{
    sum = mlsl_reduction_sum,
    prod = mlsl_reduction_prod,
    min = mlsl_reduction_min,
    max = mlsl_reduction_max,
    custom = mlsl_reduction_custom
};

/**
 * Supported mlsl data types
 */
enum class data_type
{
    dtype_char = mlsl_dtype_char,
    dtype_int = mlsl_dtype_int,
    dtype_bfp16 = mlsl_dtype_bfp16,
    dtype_float = mlsl_dtype_float,
    dtype_double = mlsl_dtype_double,
    dtype_int64 = mlsl_dtype_int64,
    dtype_uint64 = mlsl_dtype_uint64
};

/**
 * Exception type that may be thrown by mlsl API
 */
class mlsl_error : public std::runtime_error
{
public:
    explicit mlsl_error(const std::string& message) : std::runtime_error(message)
    {}

    explicit mlsl_error(const char* message) : std::runtime_error(message)
    {}
};

}
