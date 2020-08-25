#include "ccl.h"

ccl_status_t CCL_API ccl_wait(ccl_request_t req)
{
    return ccl_status_success;
}
ccl_status_t CCL_API ccl_allgatherv(
    const void* send_buf,
    size_t send_count,
    void* recv_buf,
    const size_t* recv_counts,
    ccl_datatype_t dtype,
    const ccl_coll_attr_t* attr,
    ccl_comm_t comm,
    ccl_stream_t stream,
    ccl_request_t* req)
{
    return ccl_status_success;
}

ccl_status_t CCL_API ccl_stream_create(ccl_stream_type_t type,
                               void* native_stream,
                               ccl_stream_t* stream)
{
    return ccl_status_success;
}
