#pragma once

#include <bitset>
#include <limits>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <vector>

#include "ccl_aliases.hpp"
#include "ccl_types.h"

namespace ccl {

/** API version description. */
typedef struct {
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
enum class reduction {
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
enum class datatype : int {
    int8 = 0,
    uint8,
    int16,
    uint16,
    int32,
    uint32,
    int64,
    uint64,

    bfloat8,
    bfloat16,
    float16,
    float32,
    float64,

    last_predefined
};

using datatype_attr_t = ccl_datatype_attr_t;
#ifdef DEVICE_COMM_SUPPORT
/**
 * Supported stream types
 */
enum class stream_type {
    cpu = 0,
    gpu,

    last_value
};
#endif /* DEVICE_COMM_SUPPORT */

typedef enum {
    ccl_host_color,
    ccl_host_version

} comm_split_attr_id;

template <comm_split_attr_id attrId>
struct comm_split_attributes_traits {};

/**
 * Exception type that may be thrown by ccl API
 */
class ccl_error : public std::runtime_error {
public:
    explicit ccl_error(const std::string& message) : std::runtime_error(message) {}

    explicit ccl_error(const char* message) : std::runtime_error(message) {}
};

/**
 * Type traits, which describes how-to types would be interpretered by ccl API
 */
template <class ntype_t,
          size_t size_of_type,
          ccl_datatype_t ccl_type_v,
          bool iclass = false,
          bool supported = false>
struct ccl_type_info_export {
    using native_type = ntype_t;
    using ccl_type = std::integral_constant<ccl_datatype_t, ccl_type_v>;
    static constexpr size_t size = size_of_type;
    static constexpr ccl_datatype_t ccl_type_value = ccl_type::value;
    static constexpr datatype ccl_datatype_value = static_cast<datatype>(ccl_type_value);
    static constexpr bool is_class = iclass;
    static constexpr bool is_supported = supported;
};

/**
 * API object attributes traits
 */
namespace info {
template <class param_type, param_type value>
struct param_traits {};

} //namespace info
} // namespace ccl

#ifdef DEVICE_COMM_SUPPORT
#include "ccl_device_types.hpp"
#endif
