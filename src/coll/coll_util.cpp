#include "coll/coll_util.hpp"
#include "coll/selection/selection.hpp"
#include "sched/entry/factory/entry_factory.hpp"
#if defined(CCL_ENABLE_ZE) && defined(CCL_ENABLE_SYCL)
#include "sched/entry/ze/ze_event_signal_entry.hpp"
#include "sched/entry/ze/ze_event_wait_entry.hpp"
#endif // CCL_ENABLE_ZE && CCL_ENABLE_SYCL

namespace ccl {

void add_coll_entry(ccl_sched* sched, const ccl_coll_entry_param& param) {
    ccl_selector_param selector_param;

    selector_param.ctype = param.ctype;
    selector_param.count = param.count;
    if (param.ctype == ccl_coll_allgatherv) {
        selector_param.count = param.send_count;
    }
    selector_param.recv_counts = param.recv_counts;
    selector_param.dtype = param.dtype;
    selector_param.comm = param.comm;
    selector_param.stream = param.stream;
    selector_param.buf = (param.send_buf) ? param.send_buf.get_ptr() : param.recv_buf.get_ptr();
    selector_param.is_vector_buf = sched->coll_attr.is_vector_buf;
#ifdef CCL_ENABLE_SYCL
    selector_param.is_sycl_buf = sched->coll_attr.is_sycl_buf;
#endif // CCL_ENABLE_SYCL
    selector_param.hint_algo = param.hint_algo;

    if (ccl_is_device_side_algo(selector_param)) {
        sched->strict_order = true;
    }

    if ((ccl::global_data::env().atl_transport == ccl_atl_mpi) &&
        ccl_is_direct_algo(selector_param)) {
        /* entry directly into schedule due to performance reasons */
        coll_entry::build_sched(sched, sched, param);
    }
    else {
        entry_factory::create<coll_entry>(sched, param);
    }
}

#if defined(CCL_ENABLE_ZE) && defined(CCL_ENABLE_SYCL)

void add_wait_events(ccl_sched* sched, const std::vector<ze_event_handle_t>& wait_events) {
    if (wait_events.size() > 0) {
        entry_factory::create<ze_event_wait_entry>(sched, wait_events);
        sched->add_barrier();
    }
}

void add_signal_event(ccl_sched* sched, ze_event_handle_t signal_event) {
    if (signal_event) {
        entry_factory::create<ze_event_signal_entry>(sched, signal_event);
        sched->add_barrier();
    }
}

ze_event_handle_t add_signal_event(ccl_sched* sched) {
    auto signal_event = sched->get_memory().event_manager->create();
    add_signal_event(sched, signal_event);
    return signal_event;
}

void add_comm_barrier(ccl_sched* sched,
                      ccl_comm* comm,
                      ze_event_pool_handle_t ipc_pool,
                      size_t ipc_event_idx) {
    sched->add_barrier();
    if (ipc_pool && global_data::env().enable_ze_barrier) {
        entry_factory::create<ze_barrier_entry>(sched, comm, ipc_pool, ipc_event_idx);
    }
    else {
        ccl_coll_entry_param barrier_param{};
        barrier_param.ctype = ccl_coll_barrier;
        barrier_param.comm = comm;

        /* TODO: optimize p2p based barrier */
        //barrier_param.hint_algo.barrier = ccl_coll_barrier_ring;

        add_coll_entry(sched, barrier_param);
    }
    sched->add_barrier();
}

ze_event_handle_t add_comm_barrier(ccl_sched* sched,
                                   ccl_comm* comm,
                                   const std::vector<ze_event_handle_t>& wait_events,
                                   ze_event_pool_handle_t ipc_pool,
                                   size_t ipc_event_idx) {
    sched->add_barrier();
    auto signal_event = sched->get_memory().event_manager->create();
    if (sched->use_single_list) {
        add_wait_events(sched, wait_events);
        add_comm_barrier(sched, comm, ipc_pool, ipc_event_idx);
        add_signal_event(sched, signal_event);
    }
    else {
        add_comm_barrier(sched, comm, ipc_pool, ipc_event_idx);
        add_signal_event(sched, signal_event);
    }
    sched->add_barrier();
    return signal_event;
}

void add_handle_exchange(ccl_sched* sched,
                         ccl_comm* comm,
                         const std::vector<ze_handle_exchange_entry::mem_desc_t>& in_buffers,
                         int skip_rank,
                         ze_event_pool_handle_t pool,
                         size_t event_idx) {
    if (sched->coll_attr.to_cache) {
        sched->set_entry_exec_mode(ccl_sched_entry_exec_once);
        entry_factory::create<ze_handle_exchange_entry>(sched, comm, in_buffers, skip_rank);
        sched->add_barrier();
        sched->set_entry_exec_mode(ccl_sched_entry_exec_regular);

        // TODO: no need barrier for the first iteration where ze_handle_exchange_entry exists
        add_comm_barrier(sched, comm, pool, event_idx);
    }
    else {
        entry_factory::create<ze_handle_exchange_entry>(sched, comm, in_buffers, skip_rank);
        sched->add_barrier();
    }
}

#endif // CCL_ENABLE_ZE && CCL_ENABLE_SYCL

} // namespace ccl
