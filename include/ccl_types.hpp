#pragma once

#include <bitset>
#include <limits>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <vector>

class ccl_kvs_impl;
namespace ccl
{

/** API version description. */
typedef struct
{
    unsigned int major;
    unsigned int minor;
    unsigned int update;
    const char* product_status;
    const char* build_date;
    const char* full;
} version_t;

/**
 * Supported reduction operations
 */
enum class reduction
{
    sum = 0,
    prod,
    min,
    max,
    custom,

    last_value
};

/**
 * Supported datatypes
 */
enum class datatype: int
{
    int8 = 0,
    uint8,
    int16,
    uint16,
    int32,
    uint32,
    int64,
    uint64,

    //float8,
    float16,
    bfloat16,
    float32,
    float64,

    last_value
};

/**
 * Supported stream types
 */
enum class stream_type
{
    cpu = 0,
    gpu,

    last_value
};

/**
 * Supported stream flags
 */
enum class stream_flags 
{
    default_order = 0x1U,
    in_order = 0x2U,
    out_of_order = 0x4U,

    flag_qqq = 0x8U,

    default_flags = default_order
};

template<ccl_comm_split_attributes attrId>
struct ccl_comm_split_attributes_traits {};

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
    static constexpr datatype ccl_datatype_value = static_cast<datatype>(ccl_type_value);
    static constexpr bool is_class = iclass;
    static constexpr bool is_supported = supported;
};

} // namespace ccl

#ifdef MULTI_GPU_SUPPORT
    #include "ccl_device_types.hpp"
#endif
