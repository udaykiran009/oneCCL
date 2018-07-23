#include "exec.h"
#include "global.h"
#include "log.h"
#include "mlsl.h"

mlsl_executor *executor;

mlsl_status_t mlsl_init()
{
    mlsl_env_parse();
    mlsl_comm_create(&global_comm);
    mlsl_exec_create(&executor, NULL /*transport*/);

    global_data.comm = global_comm;

    return mlsl_status_success;
}

mlsl_status_t mlsl_finalize()
{
    mlsl_comm_free(global_comm);
    mlsl_exec_free(executor);
    return mlsl_status_success;
}

size_t mlsl_get_proc_idx()
{
    return 0;
    //mlsl_transport_get_proc_idx(transport);
}

size_t mlsl_get_proc_count()
{
    return 1;
    //mlsl_transport_get_proc_count(transport);
}

size_t mlsl_get_dtype_size(mlsl_data_type_t dtype)
{
    size_t dtype_size = 1;
    switch (dtype)
    {
        case mlsl_dtype_char: { dtype_size = 1; break; }
        case mlsl_dtype_int: { dtype_size = 4; break; }
        case mlsl_dtype_bfp16: { dtype_size = 2; break; }
        case mlsl_dtype_float: { dtype_size = 4; break; }
        case mlsl_dtype_double: { dtype_size = 8; break; }
        case mlsl_dtype_int64: { dtype_size = 8; break; }
        case mlsl_dtype_unit64: { dtype_size = 8; break; }
        default: MLSL_ASSERTP(0);
    }
    return dtype_size;
}

/* TODO: move to 'collectives' module which will create/adjust/commit/start sched using sched API */
mlsl_status_t mlsl_allreduce(
    void *send_buf,
    void *recv_buf,
    size_t count,
    mlsl_data_type_t dtype,
    mlsl_reduction_t reduction,
    mlsl_request_t *req)
{
    return mlsl_status_unimplemented;
}

mlsl_status_t mlsl_wait(mlsl_request *req)
{
    return mlsl_status_unimplemented;
}

mlsl_status_t mlsl_start(mlsl_sched *sched, mlsl_request **req)
{
    return mlsl_status_unimplemented;
}

