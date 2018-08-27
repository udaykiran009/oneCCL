#ifndef COLL_H
#define COLL_H

#include "comm.h"
#include "mlsl_types.h"
#include "sched.h"

#define MLSL_INVALID_PROC_IDX (-1)

enum mlsl_coll_type
{
    mlsl_coll_barrier     = 0,
    mlsl_coll_bcast       = 1,
    mlsl_coll_reduce      = 2,
    mlsl_coll_allreduce   = 3,
    mlsl_coll_allgatherv  = 4,
    mlsl_coll_custom      = 5
};
typedef enum mlsl_coll_type mlsl_coll_type;

struct mlsl_coll_desc
{
    mlsl_coll_type ctype;
    void *buf;
    const void *send_buf;
    void *recv_buf;
    size_t count;
    size_t send_count;
    size_t *recv_counts;
    mlsl_data_type_t dtype;
    mlsl_reduction_t reduction;
    size_t root;
    mlsl_comm *comm;
};
typedef struct mlsl_coll_desc mlsl_coll_desc;

mlsl_status_t mlsl_coll_build_barrier(struct mlsl_sched *sched);
mlsl_status_t mlsl_coll_build_bcast(struct mlsl_sched *sched, void *buf, size_t count, mlsl_data_type_t dtype, size_t root);
mlsl_status_t mlsl_coll_build_reduce(struct mlsl_sched *sched, const void *send_buf, void *recv_buf, size_t count,
                                     mlsl_data_type_t dtype, mlsl_reduction_t reduction, size_t root);
mlsl_status_t mlsl_coll_build_allreduce(struct mlsl_sched *sched, const void *send_buf, void *recv_buf,
                                        size_t count, mlsl_data_type_t dtype, mlsl_reduction_t reduction);

#endif /* COLL_H */
