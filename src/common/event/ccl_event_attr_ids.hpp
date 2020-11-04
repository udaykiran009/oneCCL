#pragma once

class ccl_event;
namespace ccl {
/**
 * Event attribute ids
 */
enum class event_attr_id : int {
    version,

    native_handle,
    context,

    command_type,
    command_execution_status,

    last_value
};

} // namespace ccl
