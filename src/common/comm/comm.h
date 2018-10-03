#ifndef COMM_H
#define COMM_H

#include "mlsl_types.h"

struct mlsl_comm
{
    int ref_counter;
    size_t id;
    size_t next_sched_tag;
    size_t proc_idx;
    size_t proc_count;
    size_t pof2;
};

typedef struct mlsl_comm mlsl_comm;

/* use global comm for all operations */
extern mlsl_comm *global_comm;

mlsl_status_t mlsl_comm_create(size_t proc_idx, size_t proc_count, mlsl_comm **comm);
mlsl_status_t mlsl_comm_free(mlsl_comm *comm);

mlsl_status_t mlsl_comm_add_ref(mlsl_comm *comm);
mlsl_status_t mlsl_comm_release_ref(mlsl_comm *comm);

#endif /* COMM_H */
