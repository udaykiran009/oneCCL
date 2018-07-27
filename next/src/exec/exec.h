#ifndef EXEC_H
#define EXEC_H

#include "sched.h"
#include "worker.h"

struct mlsl_executor
{
    //struct mlsl_worker *workers;
};

typedef struct mlsl_executor mlsl_executor;

mlsl_status_t mlsl_exec_create(mlsl_executor **exec, void *t /*mlsl_transport_t transport*/);
mlsl_status_t mlsl_exec_free(mlsl_executor *exec);

mlsl_status_t mlsl_exec_start(mlsl_executor *exec, mlsl_sched *sched, mlsl_request **req);
mlsl_status_t mlsl_exec_wait(mlsl_executor *exec, mlsl_request *req);

#endif /* EXEC_H */
