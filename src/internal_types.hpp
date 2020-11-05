#pragma once

#include "oneapi/ccl/types.hpp"

namespace ccl {

// TODO: refactor core code and remove this enum?
enum status : int {
    success = 0,
    out_of_resource,
    invalid_arguments,
    runtime_error,
    blocked_due_to_resize,

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

/**
 * Supported device topology type
 */
enum device_topology_type : int {
    undetermined = -1,
    ring,
    a2a,

    last_class_value
};

} // namespace ccl
