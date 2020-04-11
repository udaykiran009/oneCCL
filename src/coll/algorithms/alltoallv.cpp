#include "coll/algorithms/algorithms.hpp"
#include "sched/entry/factory/entry_factory.hpp"

ccl_status_t ccl_coll_build_direct_alltoallv(ccl_sched* sched,
                                             ccl_buffer send_buf, const size_t* send_counts,
                                             ccl_buffer recv_buf, const size_t* recv_counts,
                                             const ccl_datatype& dtype,
                                             ccl_comm* comm)
{
    LOG_DEBUG("build direct alltoallv");

    entry_factory::make_entry<alltoallv_entry>(sched, send_buf, send_counts,
                                               recv_buf, recv_counts, dtype, comm);
    return ccl_status_success;
}
