#pragma once

#include "common/comm/comm.hpp"
#include "sched/sched.hpp"

struct mlsl_request
{
    int completion_counter;
    mlsl_sched *sched;
};

mlsl_status_t mlsl_request_create(mlsl_request **req);
mlsl_status_t mlsl_request_free(mlsl_request *req);

mlsl_status_t mlsl_request_complete(mlsl_request *req);
int mlsl_request_is_complete(mlsl_request *req);
