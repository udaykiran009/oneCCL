#pragma once

#include "oneapi/ccl/types.hpp"

namespace ccl {

enum class group_split_type : int {
    cluster,

    last_value
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

} // namespace ccl
