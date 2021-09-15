#pragma once

#include "sched/entry/coll/coll_entry.hpp"
#include "sched/entry/coll/direct/allgatherv_entry.hpp"
#include "sched/entry/coll/direct/allreduce_entry.hpp"
#include "sched/entry/coll/direct/alltoall_entry.hpp"
#include "sched/entry/coll/direct/alltoallv_entry.hpp"
#include "sched/entry/coll/direct/barrier_entry.hpp"
#include "sched/entry/coll/direct/bcast_entry.hpp"
#include "sched/entry/coll/direct/reduce_entry.hpp"
#include "sched/entry/coll/direct/reduce_scatter_entry.hpp"

#include "sched/entry/factory/entry_factory.h"

#include "sched/entry/copy/copy_entry.hpp"
#include "sched/entry/deps_entry.hpp"
#include "sched/entry/deregister_entry.hpp"
#include "sched/entry/epilogue_entry.hpp"
#include "sched/entry/function_entry.hpp"
#include "sched/entry/probe_entry.hpp"
#include "sched/entry/prologue_entry.hpp"
#include "sched/entry/recv_entry.hpp"
#include "sched/entry/recv_reduce_entry.hpp"
#include "sched/entry/reduce_local_entry.hpp"
#include "sched/entry/register_entry.hpp"
#include "sched/entry/send_entry.hpp"
#include "sched/entry/sparse_allreduce_completion_entry.hpp"
#include "sched/entry/subsched_entry.hpp"
#include "sched/entry/sync_entry.hpp"
#include "sched/entry/wait_value_entry.hpp"
#include "sched/entry/write_entry.hpp"

#if defined(MULTI_GPU_SUPPORT) && defined(CCL_ENABLE_SYCL)
#include "sched/entry/gpu/allreduce/ze_a2a_allreduce_entry.hpp"
#include "sched/entry/gpu/allreduce/ze_onesided_allreduce_entry.hpp"
#include "sched/entry/gpu/allreduce/ze_ring_allreduce_entry.hpp"
#include "sched/entry/gpu/ze_a2a_allgatherv_entry.hpp"
#include "sched/entry/gpu/ze_copy_entry.hpp"
#include "sched/entry/gpu/ze_handle_exchange_entry.hpp"
#include "sched/entry/gpu/ze_event_signal_entry.hpp"
#include "sched/entry/gpu/ze_event_wait_entry.hpp"
#include "sched/entry/gpu/ze_reduce_entry.hpp"
#endif // MULTI_GPU_SUPPORT && CCL_ENABLE_SYCL

#include "sched/sched.hpp"

namespace entry_factory {

template <class EntryType, class... Arguments>
EntryType* create(ccl_sched* sched, Arguments&&... args) {
    LOG_DEBUG("creating: ", EntryType::class_name(), " entry");
    EntryType* new_entry = detail::entry_creator<EntryType>::template make_entry<
        ccl_sched_add_mode::ccl_sched_add_mode_last_value>(sched, std::forward<Arguments>(args)...);
    LOG_DEBUG("created: ", EntryType::class_name(), ", entry: ", new_entry, ", sched: ", sched);
    return new_entry;
}

} // namespace entry_factory
