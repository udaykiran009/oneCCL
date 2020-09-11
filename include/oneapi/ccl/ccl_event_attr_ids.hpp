#pragma once

#ifndef CCL_PRODUCT_FULL
#error "Do not include this file directly. Please include 'ccl.hpp'"
#endif

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
