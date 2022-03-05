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
                                                 std::vector<ze_event_handle_t> wait_events,
                                                 size_t peer_buf_idx,
                                                 size_t peer_buf_offset)
        : ze_base_entry(sched, comm, comm->size() * event_group_count, wait_events),
          send_buf(send_buf),
          send_count(send_count),
          recv_buf(recv_buf),
          recv_counts(recv_counts, recv_counts + comm->size()),
          dtype(dtype),
          peer_buf_idx(peer_buf_idx),
          peer_buf_offset(peer_buf_offset),
          peer_count(comm->size() - 1) {}

void ze_a2a_allgatherv_entry::fill_list(const ze_base_entry* entry,
                                        int comm_rank,
                                        void* send_buf,
                                        void* recv_buf,
                                        const std::vector<ccl_buffer>& peer_recv_bufs,
                                        int peer_count,
                                        size_t copy_bytes,
                                        const ccl_datatype& dtype,
                                        size_t rank_buf_offset,
                                        bool is_inplace,
                                        std::vector<ze_event_handle_t>& copy_events,
                                        ze_event_handle_t wait_event,
                                        size_t peer_buf_offset) {
    /* copy send_buf to peer buffers */
    for (int i = 0; i < peer_count; ++i) {
        void* src = send_buf;
        if (is_inplace) {
            src = static_cast<char*>(recv_buf) + rank_buf_offset * dtype.size();
        }
        void* dst = static_cast<char*>(peer_recv_bufs[i].get_ptr()) +
                    (rank_buf_offset + peer_buf_offset) * dtype.size();
        // TODO: if we on the same device, then use t2t direction
        auto list = entry->get_copy_list(copy_direction::c2c, i);
        ZE_CALL(zeCommandListAppendMemoryCopy,
                (list, dst, src, copy_bytes, copy_events.at(i), (wait_event) ? 1 : 0, &wait_event));
    }

    if (!is_inplace) {
        /* copy send_buf to my buffer */
        void* src = send_buf;
        void* dst = static_cast<char*>(recv_buf) + rank_buf_offset * dtype.size();
        auto list = entry->get_copy_list(copy_direction::t2t);
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
        ccl_buffer buf{};
        sched->get_memory().handle_manager.get(peer_rank, peer_buf_idx, buf, comm);
        CCL_THROW_IF_NOT(buf.get_ptr(), "null IPC buffer is received");
        peer_recv_bufs[i] = buf;
    }

    bool is_inplace{};
    if (send_buf == recv_buf) {
        is_inplace = true;
    }

    size_t rank_buf_offset =
        std::accumulate(recv_counts.begin(), recv_counts.begin() + comm_rank, 0);

    size_t block_bytes =
        (!is_inplace) ? (send_count * dtype.size()) : recv_counts[comm_rank] * dtype.size();
    LOG_DEBUG("rank: ", comm_rank, ", block_bytes: ", block_bytes);

    copy_events.resize((!is_inplace) ? comm_size : peer_count);
    for (auto& event : copy_events) {
        event = ze_base_entry::create_event();
    }

    fill_list(this,
              comm_rank,
              send_buf.get_ptr(),
              recv_buf.get_ptr(),
              peer_recv_bufs,
              peer_count,
              block_bytes,
              dtype,
              rank_buf_offset,
              is_inplace,
              copy_events,
              nullptr,
              peer_buf_offset);
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

std::string ze_a2a_allgatherv_entry::name_ext() const {
    std::stringstream out;
    out << name() << " ";
    out << "send size: " << send_count;
    return out.str();
}

void ze_a2a_allgatherv_entry::dump_detail(std::stringstream& str) const {
    ccl_logger::format(str, "comm ", comm->to_string(), "\n");
}
