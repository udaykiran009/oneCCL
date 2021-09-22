#pragma once

#include "common/global/global.hpp"

namespace ccl {

void add_comm_barrier(ccl_sched* sched,
                      ccl_comm* comm,
                      ze_event_pool_handle_t pool,
                      size_t event_idx);

} // namespace ccl
