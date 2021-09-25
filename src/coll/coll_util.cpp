#include "coll_util.hpp"

#include "sched/entry/coll/coll_entry_helper.hpp"
#include "sched/entry/factory/entry_factory.hpp"

namespace ccl {

void add_comm_barrier(ccl_sched* sched,
                      ccl_comm* comm,
                      ze_event_pool_handle_t pool,
                      size_t event_idx) {
    if (pool && global_data::env().enable_ze_barrier) {
        entry_factory::create<ze_barrier_entry>(sched, comm, pool, event_idx);
    }
    else {
        ccl_coll_entry_param barrier_param{};
        barrier_param.ctype = ccl_coll_barrier;
        barrier_param.comm = comm;

        /* TODO: optimize p2p based barrier */
        //barrier_param.hint_algo.barrier = ccl_coll_barrier_ring;

        coll_entry_helper::add_coll_entry<ccl_coll_barrier>(sched, barrier_param);
    }
}

} // namespace ccl
