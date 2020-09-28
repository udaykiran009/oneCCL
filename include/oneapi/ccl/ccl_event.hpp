#pragma once

#ifndef CCL_PRODUCT_FULL
#error "Do not include this file directly. Please include 'ccl.hpp'"
#endif

namespace ccl {

class request_impl;
class event_internal;

/**
 * event's interface that allows users to track communication operation progress
 */
class event : public ccl_api_base_movable<event, direct_access_policy, request_impl> {
public:
    using base_t = ccl_api_base_movable<event, direct_access_policy, request_impl>;

    /**
     * Declare PIMPL type
     */
    using impl_value_t = typename base_t::impl_value_t;

    /**
     * Declare implementation type
     */
    using impl_t = typename impl_value_t::element_type;

    event() noexcept;
    event(event&& src) noexcept;
    event(impl_value_t&& impl) noexcept;
    ~event() noexcept;

    event& operator=(event&& src) noexcept;

    bool operator==(const event& rhs) const noexcept;
    bool operator!=(const event& rhs) const noexcept;

    /**
     * Blocking wait for operation completion
     */
    void wait();

    /**
     * Non-blocking check for operation completion
     * @retval true if the operation has been completed
     * @retval false if the operation has not been completed
     */
    bool test();

    /**
     * Cancel a pending asynchronous operation
     * @retval true if the operation has been canceled
     * @retval false if the operation has not been canceled
     */
    bool cancel();

    /**
      * Retrieve event object to be used for synchronization
      * with computation or other communication operations
      * @return event object
      */
    event_internal& get_event();
};

} // namespace ccl
