#include "common/request/request.hpp"

mlsl_status_t mlsl_request_create(mlsl_request **req)
{
    *req = static_cast<mlsl_request*>(MLSL_CALLOC(sizeof(mlsl_request), "request"));
    return mlsl_status_success;
}

mlsl_status_t mlsl_request_free(mlsl_request *req)
{
    MLSL_ASSERT(req->completion_counter == 0);
    MLSL_FREE(req);
    return mlsl_status_success;
}

mlsl_status_t mlsl_request_complete(mlsl_request *req)
{
    MLSL_ASSERT(req);
    int prev_counter __attribute__ ((unused));
    prev_counter = __atomic_fetch_sub(&(req->completion_counter), 1, __ATOMIC_RELEASE);
    MLSL_ASSERTP(prev_counter > 0);
    return mlsl_status_success;
}

int mlsl_request_is_complete(mlsl_request *req)
{
    return (req->completion_counter == 0);
}
