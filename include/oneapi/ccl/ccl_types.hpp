#pragma once

#include <stdint.h>
#include <stdlib.h>
#include "oneapi/ccl/ccl_config.h"

#include <bitset>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <vector>

#include "oneapi/ccl/ccl_aliases.hpp"
#include "oneapi/ccl/ccl_exception.hpp"

// TODO: tmp enums, refactor core code and remove them
/************************************************/
typedef int ccl_status_t;
#define ccl_status_success               (0)
#define ccl_status_out_of_resource       (1)
#define ccl_status_invalid_arguments     (2)
#define ccl_status_unimplemented         (3)
#define ccl_status_runtime_error         (4)
#define ccl_status_blocked_due_to_resize (5)
#define ccl_status_last_value            (6)
/************************************************/

namespace ccl {

/**
 * Supported reduction operations
 */
enum class reduction : int {
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

    float16,
    float32,
    float64,

    bfloat16,

    last_predefined = bfloat16
};

string_class to_string(const ccl::datatype& dt);

inline std::ostream& operator<<(std::ostream& os, const ccl::datatype& dt) {
    os << ccl::to_string(dt);
    return os;
}

typedef struct {
    const char* match_id;
    const size_t offset;
} fn_context;

/**
 * Sparse coalesce modes
 * 
 * Use this variable to set sparse_allreduce coalescing mode:
 * regular        - run regular coalesce funtion;
 * disable        - disables coalesce function in sparse_allreduce,
 *                  allgathered data is returned;
 * keep_precision - on every local reduce bf16 data is converted to fp32,
 *                  reduced and then converted back to bf16.
 */
enum class sparse_coalesce_mode : int {
    regular = 0,
    disable,
    keep_precision,

    last_value
};

/* in_buf, in_count, in_dtype, out_buf, out_count, out_dtype, context */
typedef void (*prologue_fn)(const void*,
                            size_t,
                            ccl::datatype,
                            void**,
                            size_t*,
                            ccl::datatype*,
                            const ccl::fn_context*);

/* in_buf, in_count, in_dtype, out_buf, out_count, out_dtype, context */
typedef void (*epilogue_fn)(const void*,
                            size_t,
                            ccl::datatype,
                            void*,
                            size_t*,
                            ccl::datatype,
                            const ccl::fn_context*);

/* in_buf, in_count, inout_buf, out_count, dtype, context */
typedef void (
    *reduction_fn)(const void*, size_t, void*, size_t*, ccl::datatype, const ccl::fn_context*);

/* idx_buf, idx_count, idx_dtype, val_buf, val_count, val_dtype, user_context */
typedef void (*sparse_allreduce_completion_fn)(const void*,
                                               size_t,
                                               ccl::datatype,
                                               const void*,
                                               size_t,
                                               ccl::datatype,
                                               const void*);

/* idx_count, idx_dtype, val_count, val_dtype, user_context, out_idx_buf, out_val_buf */
typedef void (*sparse_allreduce_alloc_fn)(size_t,
                                          ccl::datatype,
                                          size_t,
                                          ccl::datatype,
                                          const void*,
                                          void**,
                                          void**);

/**
 * Supported CL backend types
 */
enum class cl_backend_type : int {
    empty_backend = 0x0,
    dpcpp_sycl = 0x1,
    l0 = 0x2,
    dpcpp_sycl_l0 = 0x3,

    last_value
};

/**
 * Library version description
 */
typedef struct {
    unsigned int major;
    unsigned int minor;
    unsigned int update;
    const char* product_status;
    const char* build_date;
    const char* full;
    string_class cl_backend_name;
} library_version;

/**
 * Type traits, which describes how-to types would be interpretered by ccl API
 */
template <class ntype_t,
          size_t size_of_type,
          ccl::datatype ccl_type_v,
          bool iclass = false,
          bool supported = false>
struct ccl_type_info_export {
    using native_type = ntype_t;
    using ccl_type = std::integral_constant<ccl::datatype, ccl_type_v>;
    static constexpr size_t size = size_of_type;
    static constexpr ccl::datatype ccl_type_value = ccl_type::value;
    static constexpr datatype ccl_datatype_value = static_cast<datatype>(ccl_type_value);
    static constexpr bool is_class = iclass;
    static constexpr bool is_supported = supported;
};

struct ccl_empty_attr {
    static ccl::library_version version;

    template <class attr>
    static attr create_empty();
};

/**
 * API object attributes traits
 */
namespace info {
template <class param_type, param_type value>
struct param_traits {};

} //namespace info
} // namespace ccl

#include "oneapi/ccl/ccl_device_types.hpp"
