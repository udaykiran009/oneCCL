#include "coll/algorithms/algorithms.hpp"
#include "sched/entry/factory/entry_factory.hpp"

ccl::status ccl_coll_build_direct_alltoall(ccl_sched* sched,
                                           ccl_buffer send_buf,
                                           ccl_buffer recv_buf,
                                           size_t count,
                                           const ccl_datatype& dtype,
                                           ccl_comm* comm) {
    LOG_DEBUG("build direct alltoall");

    entry_factory::create<alltoall_entry>(sched, send_buf, recv_buf, count, dtype, comm);
    return ccl::status::success;
}
