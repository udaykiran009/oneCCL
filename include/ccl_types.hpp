#pragma once

#include "ccl_types.h"

#include <stdexcept>

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
    custom = ccl_reduction_custom,

    last_value = ccl_reduction_last_value
};

/**
 * Supported ccl data types
 */
enum class data_type
{
    dt_char = ccl_dtype_char,
    dt_int = ccl_dtype_int,
    dt_bfp16 = ccl_dtype_bfp16,
    dt_float = ccl_dtype_float,
    dt_double = ccl_dtype_double,
    dt_int64 = ccl_dtype_int64,
    dt_uint64 = ccl_dtype_uint64,

    dt_last_value = ccl_dtype_last_value
};

/**
 * Supported ccl stream types
 */
enum class stream_type
{
    cpu = ccl_stream_cpu,
    sycl = ccl_stream_sycl,

    last_value = ccl_stream_last_value
};

typedef ccl_coll_attr_t coll_attr;

typedef ccl_comm_attr_t comm_attr;

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

/**
 * Type traits, which describes how-to types would be interpretered by ccl API
 */
template<class ntype_t, size_t size_of_type, ccl_datatype_t ccl_type_v, bool iclass = false, bool supported = false>
struct ccl_type_info_export
{
    using native_type = ntype_t;
    using ccl_type = std::integral_constant<ccl_datatype_t, ccl_type_v>;
    static constexpr size_t size = size_of_type;
    static constexpr ccl_datatype_t ccl_type_value = ccl_type::value;
    static constexpr data_type ccl_datatype_value = static_cast<data_type>(ccl_type_value);
    static constexpr bool is_class = iclass;
    static constexpr bool is_supported = supported;
};
}
