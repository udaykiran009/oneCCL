#ifndef COLL_H
#define COLL_H

#include "sched.h"

mlsl_status_t mlsl_coll_build_allreduce(mlsl_sched *sched, void *send_buf, void *recv_buf,
                                        size_t count, mlsl_data_type_t dtype, mlsl_reduction_t reduction);

#endif /* COLL_H */
