#include "common/stream/stream.hpp"
#include "sched/entry/gpu/allreduce/ze_a2a_allreduce_entry.hpp"
#include "sched/entry/gpu/ze_cache.hpp"
#include "sched/entry/gpu/ze_primitives.hpp"
#include "sched/queue/queue.hpp"

#include <string>

using namespace ccl;
using namespace ccl::ze;

ze_a2a_allreduce_entry::ze_a2a_allreduce_entry(ccl_sched* sched,
                                               ccl_buffer send_buf,
                                               ccl_buffer recv_buf,
                                               size_t cnt,
                                               const ccl_datatype& dtype,
                                               reduction op,
                                               ccl_comm* comm)
        : ze_base_entry(sched, comm, comm->size() * event_group_count),
          send_buf(send_buf),
          recv_buf(recv_buf),
          dtype(dtype),
          op(op),
          buf_count(cnt),
          buf_bytes(dtype.size() * buf_count),
          peer_count(comm->size() - 1) {}

ze_a2a_allreduce_entry::~ze_a2a_allreduce_entry() {
    finalize();
}

void ze_a2a_allreduce_entry::kernel_init(size_t main_block_count,
                                         size_t block_count,
                                         void* base_ptr) {
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
        kernel_events.emplace_back(ze_base_entry::create_event());
    }

    /* reduce send_buf + tmp_buf */
    void* input_buf =
        static_cast<char*>(send_buf.get_ptr()) + comm_rank * main_block_count * dtype.size();
    void* inoutput_buf = base_ptr;
    kernels.emplace_back(module, kernel_name, worker_idx);
    kernels.back().set_args({ &count, &input_buf, &inoutput_buf });
    kernels.back().calculate_group_size(count);
    kernel_events.emplace_back(ze_base_entry::create_event());
}

void ze_a2a_allreduce_entry::init() {
    if (ze_base_entry::is_initialized) {
        return;
    }

    LOG_DEBUG("init");

    ze_base_entry::init(init_mode::compute | init_mode::copy);

    /* get peer buffers */
    std::vector<ccl_buffer> peer_send_bufs(peer_count);
    std::vector<ccl_buffer> peer_recv_bufs(peer_count);

    for (int i = 0; i < peer_count; ++i) {
        int peer_rank = (comm_rank + i + 1) % comm->size();
        sched->get_memory().handle_manager.get(peer_rank, 0, peer_send_bufs[i], comm);
        CCL_THROW_IF_NOT(peer_send_bufs[i].get_ptr(), "null IPC buffer is received");
        sched->get_memory().handle_manager.get(peer_rank, 1, peer_recv_bufs[i], comm);
        CCL_THROW_IF_NOT(peer_recv_bufs[i].get_ptr(), "null IPC buffer is received");
    }

    size_t main_block_count = buf_count / comm_size;
    if (main_block_count == 0 && static_cast<size_t>(comm_rank) < buf_count) {
        main_block_count = 1;
    }

    size_t block_count = main_block_count;
    if (comm_rank == comm_size - 1) {
        block_count += buf_count - main_block_count * comm_size;
    }

    CCL_THROW_IF_NOT(main_block_count > 0, "wrong segment count");

    /* alloc temp buffer */
    size_t tmp_buf_bytes = peer_count * block_count * dtype.size();
    ccl::alloc_param alloc_param(tmp_buf_bytes, buffer_type::ze, buffer_place::device);
    void* tmp_buf = sched->alloc_buffer(alloc_param).get_ptr();

    LOG_DEBUG("rank ",
              comm_size,
              ", main_block_count: ",
              main_block_count,
              ", block_count: ",
              block_count,
              ", tmp buf size: ",
              tmp_buf_bytes,
              ", buf_count: ",
              buf_count);

    kernel_init(main_block_count, block_count, tmp_buf);

    /* copy peer segments to temp buffer */
    size_t main_block_bytes = main_block_count * dtype.size();
    size_t block_bytes = block_count * dtype.size();
    pre_copy_events.reserve(peer_count);
    for (int i = 0; i < peer_count; ++i) {
        pre_copy_events.emplace_back(ze_base_entry::create_event());
        void* src = static_cast<char*>(peer_send_bufs[i].get_ptr()) + comm_rank * main_block_bytes;
        void* dst = static_cast<char*>(tmp_buf) + i * block_bytes;
        ZE_CALL(zeCommandListAppendMemoryCopy,
                (ze_base_entry::get_copy_list(),
                 dst,
                 src,
                 block_bytes,
                 pre_copy_events.back(),
                 0,
                 nullptr));
    }

    barrier_event = ze_base_entry::create_event();
    ZE_CALL(zeCommandListAppendBarrier,
            (ze_base_entry::get_copy_list(),
             barrier_event,
             pre_copy_events.size(),
             pre_copy_events.data()));

    /* reduce stage */
    for (size_t i = 0; i < kernels.size(); ++i) {
        ZE_CALL(zeCommandListAppendLaunchKernel,
                (ze_base_entry::comp_primitives.list,
                 kernels[i].get_kernel(),
                 kernels[i].get_group_count(),
                 kernel_events.at(i),
                 1,
                 (i == 0) ? &barrier_event : &kernel_events.at(i - 1)));
    }

    /* copy tmp_buf to peer buffers */
    post_copy_events.reserve(peer_count + 1);
    for (int i = 0; i < peer_count; ++i) {
        post_copy_events.emplace_back(ze_base_entry::create_event());
        void* src = tmp_buf;
        void* dst = static_cast<char*>(peer_recv_bufs[i].get_ptr()) + comm_rank * main_block_bytes;
        ZE_CALL(zeCommandListAppendMemoryCopy,
                (ze_base_entry::get_copy_list(),
                 dst,
                 src,
                 block_bytes,
                 post_copy_events.back(),
                 1,
                 &kernel_events.back()));
    }

    /* copy tmp_buf to my buffer */
    post_copy_events.emplace_back(ze_base_entry::create_event());
    void* src = tmp_buf;
    void* dst = static_cast<char*>(recv_buf.get_ptr()) + comm_rank * main_block_bytes;
    ZE_CALL(zeCommandListAppendMemoryCopy,
            (ze_base_entry::get_copy_list(),
             dst,
             src,
             block_bytes,
             post_copy_events.back(),
             1,
             &kernel_events.back()));

    ze_base_entry::close_lists();

    LOG_DEBUG("init completed");
}

void ze_a2a_allreduce_entry::start() {
    init();

    ze_base_entry::start();
    status = ccl_sched_entry_status_started;
}

void ze_a2a_allreduce_entry::update() {
    for (const auto& event : post_copy_events) {
        if (!ze_base_entry::is_event_completed(event)) {
            return;
        }
    }

    ZE_CALL(zeEventHostSignal, (ze_base_entry::entry_event));
    ze_base_entry::update();
    if (status == ccl_sched_entry_status_complete && !sched->coll_attr.to_cache) {
        finalize();
    }
}

void ze_a2a_allreduce_entry::finalize() {
    if (!ze_base_entry::is_initialized) {
        return;
    }

    LOG_DEBUG("finalization");

    kernels.clear();

    ze_base_entry::finalize();

    LOG_DEBUG("finalization complete");
}
