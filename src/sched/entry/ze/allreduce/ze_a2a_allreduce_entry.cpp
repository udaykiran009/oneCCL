#include "common/stream/stream.hpp"
#include "comp/comp.hpp"
#include "sched/entry/ze/allreduce/ze_a2a_allreduce_entry.hpp"
#include "sched/entry/ze/ze_a2a_allgatherv_entry.hpp"
#include "sched/entry/ze/ze_a2a_reduce_scatter_entry.hpp"
#include "sched/entry/ze/ze_cache.hpp"
#include "sched/entry/ze/ze_primitives.hpp"
#include "sched/queue/queue.hpp"

#include <algorithm>
#include <string>
#include <sstream>

using namespace ccl;
using namespace ccl::ze;

ze_a2a_allreduce_entry::ze_a2a_allreduce_entry(ccl_sched* sched,
                                               ccl_buffer send_buf,
                                               ccl_buffer recv_buf,
                                               size_t cnt,
                                               const ccl_datatype& dtype,
                                               reduction op,
                                               ccl_comm* comm,
                                               std::vector<ze_event_handle_t> wait_events,
                                               size_t send_buf_idx,
                                               size_t recv_buf_idx,
                                               size_t peer_buf_offset)
        : ze_base_entry(sched, comm, comm->size() * event_group_count, wait_events),
          send_buf(send_buf),
          recv_buf(recv_buf),
          cnt(cnt),
          dtype(dtype),
          op(op),
          send_buf_idx(send_buf_idx),
          recv_buf_idx(recv_buf_idx),
          peer_buf_offset(peer_buf_offset),
          peer_count(comm->size() - 1) {
    size_t segment_count = cnt / comm->size();
    bool count_check =
        (segment_count > 0) || (segment_count == 0 && static_cast<size_t>(comm->rank()) < cnt);
    skip_entry = !count_check || ((comm->size() == 1) && (send_buf == recv_buf));
    if (skip_entry) {
        // skip entry init and finalize
        sched->ze_entries.pop_back();
    }
}

void ze_a2a_allreduce_entry::init_ze_hook() {
    /* get peer buffers */
    std::vector<ccl_buffer> peer_send_bufs(peer_count);
    std::vector<ccl_buffer> peer_recv_bufs(peer_count);

    for (int i = 0; i < peer_count; ++i) {
        int peer_rank = (comm_rank + i + 1) % comm->size();
        sched->get_memory().handle_manager.get(peer_rank, send_buf_idx, peer_send_bufs[i], comm);
        CCL_THROW_IF_NOT(peer_send_bufs[i].get_ptr(), "null IPC buffer is received");
        sched->get_memory().handle_manager.get(peer_rank, recv_buf_idx, peer_recv_bufs[i], comm);
        CCL_THROW_IF_NOT(peer_recv_bufs[i].get_ptr(), "null IPC buffer is received");
    }

    size_t main_block_count = cnt / comm_size;
    if (main_block_count == 0 && static_cast<size_t>(comm_rank) < cnt) {
        main_block_count = 1;
    }

    size_t block_count = main_block_count;
    if (comm_rank == comm_size - 1) {
        block_count += cnt - main_block_count * comm_size;
    }

    CCL_THROW_IF_NOT(main_block_count > 0, "wrong segment count");

    /* alloc temp buffer */
    size_t tmp_buf_bytes = peer_count * block_count * dtype.size();
    ccl::alloc_param alloc_param(tmp_buf_bytes, buffer_type::ze, buffer_place::device);
    void* tmp_buf = sched->alloc_buffer(alloc_param).get_ptr();

    LOG_DEBUG("rank ",
              comm_rank,
              ", main_block_count: ",
              main_block_count,
              ", block_count: ",
              block_count,
              ", tmp buf size: ",
              tmp_buf_bytes,
              ", cnt: ",
              cnt);

    /* copy peer segments to temp buffer */
    size_t block_bytes = block_count * dtype.size();

    pre_copy_events.resize(peer_count);
    for (auto& event : pre_copy_events) {
        event = ze_base_entry::create_event();
    }

    kernel_events.resize(peer_count);
    for (auto& event : kernel_events) {
        event = ze_base_entry::create_event();
    }

    barrier_event = ze_base_entry::create_event();
    ze_a2a_reduce_scatter_entry::fill_list(this,
                                           send_buf.get_ptr(),
                                           tmp_buf,
                                           peer_send_bufs,
                                           peer_count,
                                           comm_rank,
                                           block_count,
                                           comm_rank * main_block_count,
                                           pre_copy_events,
                                           kernels,
                                           kernel_events,
                                           barrier_event,
                                           dtype,
                                           module,
                                           device,
                                           context,
                                           op,
                                           worker_idx,
                                           peer_buf_offset);

    post_copy_events.resize(comm_size);
    for (auto& event : post_copy_events) {
        event = ze_base_entry::create_event();
    }
    ze_a2a_allgatherv_entry::fill_list(this,
                                       comm_rank,
                                       tmp_buf,
                                       recv_buf.get_ptr(),
                                       peer_recv_bufs,
                                       peer_count,
                                       block_bytes,
                                       dtype,
                                       comm_rank * main_block_count,
                                       false,
                                       post_copy_events,
                                       kernel_events.back(),
                                       peer_buf_offset);
}

void ze_a2a_allreduce_entry::start() {
    if (skip_entry) {
        ZE_CALL(zeEventHostSignal, (ze_base_entry::entry_event));
        status = ccl_sched_entry_status_complete;
        return;
    }

    ze_base_entry::start();
}

void ze_a2a_allreduce_entry::update() {
    for (const auto& event : post_copy_events) {
        if (!ze_base_entry::is_event_completed(event)) {
            return;
        }
    }

    ZE_CALL(zeEventHostSignal, (ze_base_entry::entry_event));
    ze_base_entry::update();
}

std::string ze_a2a_allreduce_entry::name_ext() const {
    std::stringstream out;
    out << name() << " ";
    out << "size: " << cnt;
    return out.str();
}

void ze_a2a_allreduce_entry::dump_detail(std::stringstream& str) const {
    ccl_logger::format(str,
                       "dt ",
                       ccl::global_data::get().dtypes->name(dtype),
                       ", cnt ",
                       cnt,
                       ", send_buf ",
                       send_buf,
                       ", recv_buf ",
                       recv_buf,
                       ", op ",
                       ccl_reduction_to_str(op),
                       ", comm ",
                       comm->to_string(),
                       ", context ",
                       context,
                       "\n");
}
