#pragma once

#include "ccl_types.h"

#include <stdexcept>

#ifdef ENABLE_SYCL
#include <CL/sycl.hpp>
typedef cl::sycl::buffer<char, 1> ccl_sycl_buffer_t;
#endif /* ENABLE_SYCL */

namespace ccl
{

/**
 * Supported ccl reduction types
 */
enum class reduction
{
    sum = ccl_reduction_sum,
    prod = ccl_reduction_prod,
    min = ccl_reduction_min,
    max = ccl_reduction_max,
    custom = ccl_reduction_custom
};

/**
 * Supported ccl data types
 */
enum class data_type
{
    dtype_char = ccl_dtype_char,
    dtype_int = ccl_dtype_int,
    dtype_bfp16 = ccl_dtype_bfp16,
    dtype_float = ccl_dtype_float,
    dtype_double = ccl_dtype_double,
    dtype_int64 = ccl_dtype_int64,
    dtype_uint64 = ccl_dtype_uint64
};

/**
 * Exception type that may be thrown by ccl API
 */
class ccl_error : public std::runtime_error
{
public:
    explicit ccl_error(const std::string& message) : std::runtime_error(message)
    {}

    explicit ccl_error(const char* message) : std::runtime_error(message)
    {}
};

}
