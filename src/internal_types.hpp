#pragma once

namespace ccl {

// TODO: refactor core code and remove this enum?
enum status : int {
    success = 0,
    out_of_resource,
    invalid_arguments,
    unimplemented,
    runtime_error,
    blocked_due_to_resize,

    last_value
};

} // namespace ccl
