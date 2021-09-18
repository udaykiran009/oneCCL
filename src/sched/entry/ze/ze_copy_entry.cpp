#include "sched/entry/ze/ze_copy_entry.hpp"

using namespace ccl;

ze_copy_entry::ze_copy_entry(ccl_sched* sched,
                             ccl_buffer in_buf,
                             ccl_buffer out_buf,
                             size_t count,
                             const ccl_datatype& dtype,
                             copy_attr attr)
        : ze_base_entry(sched, init_mode::copy),
          sched(sched),
          in_buf(in_buf),
          out_buf(out_buf),
          dtype(dtype),
          attr(attr),
          buf_size_bytes(dtype.size() * count) {
    CCL_THROW_IF_NOT(sched, "no sched");
}

void ze_copy_entry::init_ze_hook() {
    if (attr.peer_rank != ccl_comm::invalid_rank) {
        if (!out_buf) {
            sched->get_memory().handle_manager.get(
                attr.peer_rank, attr.peer_buf_idx, out_buf, attr.map_comm);
        }

        if (!in_buf) {
            sched->get_memory().handle_manager.get(
                attr.peer_rank, attr.peer_buf_idx, in_buf, attr.map_comm);
        }
    }

    void* dst = out_buf.get_ptr();
    void* src = static_cast<char*>(in_buf.get_ptr()) + attr.in_buf_offset * dtype.size();
    ze_command_list_handle_t list = ze_base_entry::get_copy_list();

    ZE_CALL(zeCommandListAppendMemoryCopy,
            (list, dst, src, buf_size_bytes, ze_base_entry::entry_event, 0, nullptr));
}
