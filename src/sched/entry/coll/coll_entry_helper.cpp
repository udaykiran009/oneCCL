#include "sched/entry/coll/coll_entry_helper.hpp"
#include "sched/entry/subsched_entry.hpp"

ccl::status coll_entry_helper::build_schedule(ccl_sched* sched,
                                              const ccl_sched* parent_sched,
                                              const ccl_coll_entry_param& param) {
    ccl::status res = ccl::status::success;

    subsched_entry::inherit_params(sched, parent_sched, param.ctype);

    sched->hint_algo = param.hint_algo;

    switch (param.ctype) {
        case ccl_coll_allgatherv: {
            res = ccl_coll_build_allgatherv(sched,
                                            param.send_buf,
                                            param.send_count,
                                            param.recv_buf,
                                            param.recv_counts,
                                            param.dtype,
                                            param.comm);
            break;
        }
        case ccl_coll_allreduce: {
            res = ccl_coll_build_allreduce(sched,
                                           param.send_buf,
                                           param.recv_buf,
                                           param.count,
                                           param.dtype,
                                           param.reduction,
                                           param.comm);
            break;
        }
        case ccl_coll_alltoall: {
            res = ccl_coll_build_alltoall(
                sched, param.send_buf, param.recv_buf, param.count, param.dtype, param.comm);
            break;
        }
        case ccl_coll_alltoallv: {
            res = ccl_coll_build_alltoallv(sched,
                                           param.send_buf,
                                           param.send_counts,
                                           param.recv_buf,
                                           param.recv_counts,
                                           param.dtype,
                                           param.comm);
            break;
        }
        case ccl_coll_barrier: {
            res = ccl_coll_build_barrier(sched, param.comm);
            break;
        }
        case ccl_coll_bcast: {
            res = ccl_coll_build_bcast(
                sched, param.recv_buf, param.count, param.dtype, param.root, param.comm);
            break;
        }
        case ccl_coll_reduce: {
            res = ccl_coll_build_reduce(sched,
                                        param.send_buf,
                                        param.recv_buf,
                                        param.count,
                                        param.dtype,
                                        param.reduction,
                                        param.root,
                                        param.comm);
            break;
        }
        case ccl_coll_reduce_scatter: {
            res = ccl_coll_build_reduce_scatter(sched,
                                                param.send_buf,
                                                param.recv_buf,
                                                param.count,
                                                param.dtype,
                                                param.reduction,
                                                param.comm);
            break;
        }
        default: CCL_FATAL("not supported type ", param.ctype); break;
    }
    return res;
}
