#pragma once

#include "common/comm/comm.hpp"
#include "sched/sched.hpp"

// TODO: use request allocator

struct mlsl_request
{
    int ref_counter;
    int completion_counter;
    struct mlsl_sched *sched;
    struct mlsl_comm *comm;
};

mlsl_status_t mlsl_request_create(mlsl_request **req);
mlsl_status_t mlsl_request_free(mlsl_request *req);

mlsl_status_t mlsl_request_add_ref(mlsl_request *req);
mlsl_status_t mlsl_request_release_ref(mlsl_request *req);

mlsl_status_t mlsl_request_complete(mlsl_request *req);
int mlsl_request_is_complete(mlsl_request *req);
