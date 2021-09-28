#pragma once

#include "common/global/global.hpp"
#include "sched/entry/ze/ze_handle_exchange_entry.hpp"

namespace ccl {

void add_comm_barrier(ccl_sched* sched,
                      ccl_comm* comm,
                      ze_event_pool_handle_t pool = nullptr,
                      size_t event_idx = 0);

void add_handle_exchange(ccl_sched* sched,
                         ccl_comm* comm,
                         const std::vector<ze_handle_exchange_entry::mem_desc_t>& in_buffers,
                         int skip_rank = ccl_comm::invalid_rank);

} // namespace ccl
