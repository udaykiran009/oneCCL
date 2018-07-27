#include "exec.h"

mlsl_status_t mlsl_exec_create(mlsl_executor **executor, void *t/*mlsl_transport_t transport*/)
{
    return mlsl_status_unimplemented;
}

mlsl_status_t mlsl_exec_free(mlsl_executor *executor)
{
    return mlsl_status_unimplemented;
}

mlsl_status_t mlsl_exec_start(mlsl_executor *executor, mlsl_sched *sched, mlsl_request **req)
{
    return mlsl_status_unimplemented;
}

mlsl_status_t mlsl_exec_wait(mlsl_executor *executor, mlsl_request *req)
{
    return mlsl_status_unimplemented;
}
