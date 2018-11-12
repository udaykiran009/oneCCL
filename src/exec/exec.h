#ifndef EXEC_H
#define EXEC_H

#ifdef __cplusplus
extern "C" {
#endif

#include "sched/sched.h"
#include "atl/atl.h"
#include "exec/worker.h"

struct mlsl_executor
{
    atl_desc_t *atl_desc;
    atl_comm_t **atl_comms;

    // TODO_ATL: add corresponding ATL methods like MPI_Comm_rank/size
    size_t proc_idx;
    size_t proc_count;

    size_t worker_count;
    struct mlsl_worker **workers;
};

typedef struct mlsl_executor mlsl_executor;

extern mlsl_executor *global_executor;

mlsl_status_t mlsl_executor_create(size_t worker_count, size_t priority_count, mlsl_executor **executor);
mlsl_status_t mlsl_executor_free(mlsl_executor *executor);
mlsl_status_t mlsl_executor_start(mlsl_executor *executor, mlsl_sched *sched);
mlsl_status_t mlsl_executor_wait(mlsl_executor *executor, mlsl_request *req);
mlsl_status_t mlsl_executor_test(mlsl_executor *executor, mlsl_request *req, int *is_completed);

#ifdef __cplusplus
}
#endif

#endif /* EXEC_H */
