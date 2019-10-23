Tracking Communication Progress
===============================

The progress for any of the collective operations provided by oneCCL can be tracked using two functions: `Test`_  and `Wait`_

Test
****

Non-blocking operation that returns the completion status.

**C version of oneCCL API:**

::

    ccl_status_t ccl_test(ccl_request_t req, int* is_completed);

req
    requests the handle for communication operation being tracked
is_completed
    indicates the status: 0 - operation is in progress; otherwise, the operation is completed

**C++ version of oneCCL API:**

::

    bool request::test();

Returnes the value that indicates the status: 0 - operation is in progress; otherwise, the operation is completed.

Wait
****

Operation that blocks the execution until communication operation is completed.

**C version of oneCCL API:**

::

    ccl_status_t ccl_wait(ccl_request_t req);

req
    requests the handle for communication operation being tracked

**C++ version of oneCCL API:**

::

    void request::wait();
