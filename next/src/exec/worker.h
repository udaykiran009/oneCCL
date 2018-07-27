#ifndef WORKER_H
#define WORKER_H

#include "exec.h"

struct mlsl_worker_context
{
	// comm_ctx
	// comp_ctx
	// priority_sched_queue
};
typedef struct mlsl_worker_context mlsl_worker_context;

struct mlsl_worker
{
	size_t idx;
	struct mlsl_executor *executor;
	mlsl_worker_context *ctx;
};

typedef struct mlsl_worker mlsl_worker;

mlsl_status_t mlsl_worker_create(mlsl_worker **worker);
mlsl_status_t mlsl_worker_free(mlsl_worker *worker);

mlsl_status_t mlsl_worker_start(mlsl_worker *worker);
mlsl_status_t mlsl_worker_wait(mlsl_worker *worker);

#endif /* WORKER_H */
