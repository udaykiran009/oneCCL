#pragma once

#include "oneapi/ccl/types.hpp"

namespace ccl {

// Specifies the type of the communicator that should be created
// TODO: check whether we need to keep the commented out values
// TODO: find a better name for the enum? Right now we have another
// public enum that represents split type
enum class group_split_type : int { // TODO fill in this enum with the actual values
    // Use negative numbers for the 2 values to differentiate them from other 3 cases which represent
    // topology type in a generic code, for single we have a distinct handling
    // TODO: may refactor into 2 enums? 1 for determining the communicator type and the other one for
    // topology?
    // Not deduced/default
    undetermined = -2,
    // Single device communicator: work only with a single device
    single = -1,
    //device,
    // Thread communicator: 1 thread shares multiple devices
    thread,
    // Process communicator: multiple threads share multiple devices
    process,
    //socket,
    //node,
    // Cluster communicator: multiple processes share multiple devices
    cluster,

    last_value
};

/**
 * Supported device topology type
 */
enum device_topology_type : int {
    undetermined = -1,
    ring,
    a2a,

    last_class_value
};

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

} // namespace ccl
