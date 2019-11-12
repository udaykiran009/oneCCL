#include "sched/entry/coll/coll_entry_helper.hpp"

ccl_status_t coll_entry_helper::build_schedule(ccl_sched* sched,
                                               const ccl_sched* parent_sched,
                                               const ccl_coll_entry_param& param)
{
    ccl_status_t res = ccl_status_success;

    switch (param.ctype)
    {
        case ccl_coll_allgatherv:
        {
            res = ccl_coll_build_allgatherv(sched,
                                            param.send_buf,
                                            param.send_count,
                                            param.recv_buf,
                                            param.recv_counts,
                                            param.dtype);
            break;
        }
        case ccl_coll_allreduce:
        {
            if (sched != parent_sched)
            {
                sched->coll_attr.reduction_fn = parent_sched->coll_attr.reduction_fn;
                /* required to create ccl_fn_context in reduce/recv_reduce entries */
                sched->coll_attr.match_id = parent_sched->coll_attr.match_id;
            }

            res = ccl_coll_build_allreduce(sched,
                                           param.send_buf,
                                           param.recv_buf,
                                           param.count,
                                           param.dtype,
                                           param.reduction);
            break;
        }
        case ccl_coll_alltoall:
        {
            res = ccl_coll_build_alltoall(sched,
                                          param.send_buf,
                                          param.recv_buf,
                                          param.count,
                                          param.dtype);
            break;
        }
        case ccl_coll_barrier:
        {
            res = ccl_coll_build_barrier(sched);
            break;
        }
        case ccl_coll_bcast:
        {
            res = ccl_coll_build_bcast(sched,
                                       param.buf,
                                       param.count,
                                       param.dtype,
                                       param.root);
            break;
        }
        case ccl_coll_reduce:
        {
            if (sched != parent_sched)
            {
                sched->coll_attr.reduction_fn = parent_sched->coll_attr.reduction_fn;
                /* required to create ccl_fn_context in reduce/recv_reduce entries */
                sched->coll_attr.match_id = parent_sched->coll_attr.match_id;
            }

            res = ccl_coll_build_reduce(sched,
                                        param.send_buf,
                                        param.recv_buf,
                                        param.count,
                                        param.dtype,
                                        param.reduction,
                                        param.root);
            break;
        }
        default:
            CCL_FATAL("not supported type ", param.ctype);
            break;
    }
    return res;
}
