#include "sched/entry/ze/ze_a2a_reduce_scatter_entry.hpp"
#include "sched/entry/ze/ze_cache.hpp"
#include "sched/entry/ze/ze_primitives.hpp"

#include <numeric>

using namespace ccl;
using namespace ccl::ze;

ze_a2a_reduce_scatter_entry::ze_a2a_reduce_scatter_entry(ccl_sched* sched,
                                                         ccl_buffer send_buf,
                                                         ccl_buffer recv_buf,
                                                         size_t cnt,
                                                         const ccl_datatype& dtype,
                                                         reduction op,
                                                         ccl_comm* comm,
                                                         size_t peer_buf_idx)
        : ze_base_entry(sched,
                        (init_mode::compute | init_mode::copy),
                        comm,
                        comm->size() * event_group_count),
          send_buf(send_buf),
          recv_buf(recv_buf),
          dtype(dtype),
          op(op),
          buf_count(cnt),
          buf_bytes(dtype.size() * buf_count),
          peer_buf_idx(peer_buf_idx),
          peer_count(comm->size() - 1) {}

void ze_a2a_reduce_scatter_entry::kernel_init(size_t main_block_count,
                                              size_t block_count,
                                              void* send_buf,
                                              void* base_ptr,
                                              int peer_count,
                                              const ccl_datatype& dtype,
                                              int comm_rank,
                                              std::vector<ze_kernel>& kernels,
                                              ze_module_handle_t module,
                                              ze_device_handle_t device,
                                              ze_context_handle_t context,
                                              ccl::reduction op,
                                              size_t worker_idx) {
    global_data::get().ze_cache->get(context, device, "kernels.spv", &module);
    std::string kernel_name =
        "reduce_local_inplace_kernel_" + to_string(dtype.idx()) + "_" + ccl_reduction_to_str(op);

    /* reduce peer values in tmp_buf only */
    kernels.reserve(peer_count);
    unsigned long count = block_count;
    for (int i = 1; i < peer_count; ++i) {
        void* input_buf = static_cast<char*>(base_ptr) + i * block_count * dtype.size();
        void* inoutput_buf = base_ptr;
        kernels.emplace_back(module, kernel_name, worker_idx);
        kernels.back().set_args({ &count, &input_buf, &inoutput_buf });
        kernels.back().calculate_group_size(count);
    }

    /* reduce send_buf + tmp_buf */
    void* input_buf = static_cast<char*>(send_buf) + comm_rank * main_block_count * dtype.size();
    void* inoutput_buf = base_ptr;
    kernels.emplace_back(module, kernel_name, worker_idx);
    kernels.back().set_args({ &count, &input_buf, &inoutput_buf });
    kernels.back().calculate_group_size(count);
}

void ze_a2a_reduce_scatter_entry::fill_list(ze_command_list_handle_t copy_list,
                                            ze_command_list_handle_t comp_list,
                                            void* send_buf,
                                            void* tmp_buf,
                                            const std::vector<ccl_buffer>& peer_send_bufs,
                                            int peer_count,
                                            int comm_rank,
                                            size_t main_block_count,
                                            size_t block_count,
                                            size_t copy_bytes,
                                            size_t offset_bytes,
                                            std::vector<ze_event_handle_t>& copy_events,
                                            std::vector<ze_kernel>& kernels,
                                            std::vector<ze_event_handle_t>& kernel_events,
                                            ze_event_handle_t& barrier_event,
                                            const ccl_datatype& dtype,
                                            ze_module_handle_t module,
                                            ze_device_handle_t device,
                                            ze_context_handle_t context,
                                            ccl::reduction op,
                                            size_t worker_idx) {
    kernel_init(main_block_count,
                block_count,
                send_buf,
                tmp_buf,
                peer_count,
                dtype,
                comm_rank,
                kernels,
                module,
                device,
                context,
                op,
                worker_idx);
    /* copy peer segments to temp buffer */
    for (int i = 0; i < peer_count; i++) {
        void* src = static_cast<char*>(peer_send_bufs[i].get_ptr()) + offset_bytes;
        void* dst = static_cast<char*>(tmp_buf) + i * copy_bytes;
        ZE_CALL(zeCommandListAppendMemoryCopy,
                (copy_list, dst, src, copy_bytes, copy_events.at(i), 0, nullptr));
    }

    ZE_CALL(zeCommandListAppendBarrier,
            (copy_list, barrier_event, copy_events.size(), copy_events.data()));

    /* reduce stage */
    for (size_t i = 0; i < kernels.size(); ++i) {
        ZE_CALL(zeCommandListAppendLaunchKernel,
                (comp_list,
                 kernels[i].get_kernel(),
                 kernels[i].get_group_count(),
                 kernel_events.at(i),
                 1,
                 (i == 0) ? &barrier_event : &kernel_events.at(i - 1)));
    }
}

void ze_a2a_reduce_scatter_entry::init_ze_hook() {
    /* get peer buffers */
    std::vector<ccl_buffer> peer_send_bufs(peer_count);

    for (int i = 0; i < peer_count; ++i) {
        int peer_rank = (comm_rank + i + 1) % comm->size();
        sched->get_memory().handle_manager.get(peer_rank, peer_buf_idx, peer_send_bufs[i], comm);
        CCL_THROW_IF_NOT(peer_send_bufs[i].get_ptr(), "null IPC buffer is received");
    }

    /* alloc temp buffer */
    size_t tmp_buf_bytes = peer_count * buf_bytes;
    ccl::alloc_param alloc_param(tmp_buf_bytes, buffer_type::ze, buffer_place::device);
    void* tmp_buf = sched->alloc_buffer(alloc_param).get_ptr();

    LOG_DEBUG("rank ", comm_size, ", tmp buf size: ", tmp_buf_bytes, ", buf_count: ", buf_count);

    /* copy peer segments to temp buffer */

    pre_copy_events.resize(peer_count);
    for (auto& event : pre_copy_events) {
        event = ze_base_entry::create_event();
    }

    kernel_events.resize(peer_count);
    for (auto& event : kernel_events) {
        event = ze_base_entry::create_event();
    }

    barrier_event = ze_base_entry::create_event();

    fill_list(ze_base_entry::get_copy_list(),
              ze_base_entry::comp_primitives.list,
              send_buf.get_ptr(),
              tmp_buf,
              peer_send_bufs,
              peer_count,
              comm_rank,
              buf_count,
              buf_count,
              buf_bytes,
              comm_rank * buf_bytes,
              pre_copy_events,
              kernels,
              kernel_events,
              barrier_event,
              dtype,
              module,
              device,
              context,
              op,
              worker_idx);
    post_copy_events.resize(1);
    for (auto& event : post_copy_events) {
        event = ze_base_entry::create_event();
    }
    ZE_CALL(zeCommandListAppendMemoryCopy,
            (ze_base_entry::get_copy_list(),
             recv_buf.get_ptr(),
             tmp_buf,
             buf_bytes,
             post_copy_events.back(),
             1,
             &kernel_events.back()));
}

void ze_a2a_reduce_scatter_entry::update() {
    for (const auto& event : post_copy_events) {
        if (!ze_base_entry::is_event_completed(event)) {
            return;
        }
    }
    ZE_CALL(zeEventHostSignal, (ze_base_entry::entry_event));
    ze_base_entry::update();
}
