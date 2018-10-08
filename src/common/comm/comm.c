#include "common/comm/comm.h"
#include "common/global/global.h"
#include "common/utils/utils.h"

mlsl_comm *global_comm = NULL;

mlsl_status_t mlsl_comm_create(size_t proc_idx, size_t proc_count, mlsl_comm **comm)
{
    mlsl_comm *c = MLSL_CALLOC(sizeof(mlsl_comm), "comm");
    c->next_sched_tag = MLSL_TAG_FIRST;
    c->proc_idx = proc_idx;
    c->proc_count = proc_count;
    c->pof2 = mlsl_pof2(proc_count);
    *comm = c;
    return mlsl_status_success;
}

mlsl_status_t mlsl_comm_free(mlsl_comm *comm)
{
    MLSL_FREE(comm);
    return mlsl_status_success;
}

mlsl_status_t mlsl_comm_add_ref(mlsl_comm *comm)
{
    return mlsl_status_unimplemented;
}

mlsl_status_t mlsl_comm_release_ref(mlsl_comm *comm)
{
    return mlsl_status_unimplemented;
}
