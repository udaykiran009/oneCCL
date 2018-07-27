#ifndef REQUEST_H
#define REQUEST_H

#include "comm.h"
#include "sched.h"

// TODO: use request allocator

struct mlsl_request
{
    int ref_counter;
    int completion_counter;
    struct mlsl_sched *sched;
    struct mlsl_comm *comm;
};

typedef struct mlsl_request mlsl_request;

mlsl_status_t mlsl_request_create(mlsl_request **req);
mlsl_status_t mlsl_request_free(mlsl_request *req);

mlsl_status_t mlsl_request_add_ref(mlsl_request *req);
mlsl_status_t mlsl_request_release_ref(mlsl_request *req);

mlsl_status_t mlsl_request_complete(mlsl_request *req);
int mlsl_request_is_complete(mlsl_request *req);

#endif /* REQUEST_H */
