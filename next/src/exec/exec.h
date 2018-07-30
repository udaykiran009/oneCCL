#ifndef EXEC_H
#define EXEC_H

#include "sched.h"
#include "worker.h"

struct mlsl_executor
{
    size_t worker_count;
    struct mlsl_worker **workers;
};

typedef struct mlsl_executor mlsl_executor;

extern mlsl_executor *global_executor;

mlsl_status_t mlsl_executor_create(size_t worker_count, size_t priority_count, mlsl_executor **executor);
mlsl_status_t mlsl_executor_free(mlsl_executor *executor);
mlsl_status_t mlsl_executor_start(mlsl_executor *executor, mlsl_sched *sched, mlsl_request **req);
mlsl_status_t mlsl_executor_wait(mlsl_executor *executor, mlsl_request *req);

#endif /* EXEC_H */
