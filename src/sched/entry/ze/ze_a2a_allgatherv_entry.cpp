#include "sched/entry/ze/ze_a2a_allgatherv_entry.hpp"
#include "sched/entry/ze/ze_cache.hpp"
#include "sched/entry/ze/ze_primitives.hpp"

#include <numeric>

using namespace ccl;
using namespace ccl::ze;

ze_a2a_allgatherv_entry::ze_a2a_allgatherv_entry(ccl_sched* sched,
                                                 ccl_buffer send_buf,
                                                 size_t send_count,
                                                 ccl_buffer recv_buf,
                                                 const size_t* recv_counts,
                                                 const ccl_datatype& dtype,
                                                 ccl_comm* comm,
                                                 size_t peer_buf_idx)
        : ze_base_entry(sched, init_mode::copy, comm, comm->size() * event_group_count),
          send_buf(send_buf),
          send_bytes(send_count * dtype.size()),
          recv_buf(recv_buf),
          recv_counts(recv_counts, recv_counts + comm->size()),
          dtype(dtype),
          peer_buf_idx(peer_buf_idx),
          peer_count(comm->size() - 1) {}

void ze_a2a_allgatherv_entry::fill_list(ze_command_list_handle_t list,
                                        void* send_buf,
                                        void* recv_buf,
                                        const std::vector<ccl_buffer>& peer_recv_bufs,
                                        int peer_count,
                                        size_t copy_bytes,
                                        size_t offset_bytes,
                                        bool is_inplace,
                                        std::vector<ze_event_handle_t>& copy_events,
                                        ze_event_handle_t wait_event) {
    /* copy send_buf to peer buffers */
    for (int i = 0; i < peer_count; ++i) {
        void* src = send_buf;
        if (is_inplace) {
            src = static_cast<char*>(recv_buf) + offset_bytes;
        }
        void* dst = static_cast<char*>(peer_recv_bufs[i].get_ptr()) + offset_bytes;
        ZE_CALL(zeCommandListAppendMemoryCopy,
                (list, dst, src, copy_bytes, copy_events.at(i), (wait_event) ? 1 : 0, &wait_event));
    }

    if (!is_inplace) {
        /* copy send_buf to my buffer */
        void* src = send_buf;
        void* dst = static_cast<char*>(recv_buf) + offset_bytes;
        ZE_CALL(
            zeCommandListAppendMemoryCopy,
            (list, dst, src, copy_bytes, copy_events.back(), (wait_event) ? 1 : 0, &wait_event));
    }
}

void ze_a2a_allgatherv_entry::init_ze_hook() {
    /* get peer recv buffers */
    std::vector<ccl_buffer> peer_recv_bufs(peer_count);

    for (int i = 0; i < peer_count; ++i) {
        int peer_rank = (comm_rank + i + 1) % comm->size();
        sched->get_memory().handle_manager.get(peer_rank, peer_buf_idx, peer_recv_bufs[i], comm);
        CCL_THROW_IF_NOT(peer_recv_bufs[i].get_ptr(), "null IPC buffer is received");
    }

    bool is_inplace{};
    if (send_buf == recv_buf) {
        is_inplace = true;
    }

    size_t offset_count = std::accumulate(recv_counts.begin(), recv_counts.begin() + comm_rank, 0);
    size_t offset_bytes = offset_count * dtype.size();
    size_t block_bytes = (!is_inplace) ? send_bytes : recv_counts[comm_rank] * dtype.size();
    LOG_DEBUG("rank: ", comm_rank, ", block_bytes: ", block_bytes);

    copy_events.resize((!is_inplace) ? comm_size : peer_count);
    for (auto& event : copy_events) {
        event = ze_base_entry::create_event();
    }

    fill_list(ze_base_entry::get_copy_list(),
              send_buf.get_ptr(),
              recv_buf.get_ptr(),
              peer_recv_bufs,
              peer_count,
              block_bytes,
              offset_bytes,
              is_inplace,
              copy_events);
}

void ze_a2a_allgatherv_entry::update() {
    for (const auto& event : copy_events) {
        if (!ze_base_entry::is_event_completed(event)) {
            return;
        }
    }

    ZE_CALL(zeEventHostSignal, (ze_base_entry::entry_event));
    ze_base_entry::update();
}
