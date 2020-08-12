#pragma once

#ifndef CCL_PRODUCT_FULL
#error "Do not include this file directly. Please include 'ccl.hpp'"
#endif

namespace ccl {

class event;

/**
 * Request's interface that allows users to track communication operation progress
 */
class request {
public:
    /**
     * Blocking wait for operation completion
     */
    virtual void wait() = 0;

    /**
     * Non-blocking check for operation completion
     * @retval true if the operation has been completed
     * @retval false if the operation has not been completed
     */
    virtual bool test() = 0;

    /**
     * Cancel a pending asynchronous operation
     * @retval true if the operation has been canceled
     * @retval false if the operation has not been canceled
     */
    virtual bool cancel() = 0;

    /**
      * Retrieve event object to be used for synchronization
      * with computation or other communication operations
      * @return event object
      */
    virtual event& get_event() = 0;

    virtual ~request() = default;
};

}
