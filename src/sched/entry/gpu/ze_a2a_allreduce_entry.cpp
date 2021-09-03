#include "common/stream/stream.hpp"
#include "sched/entry/gpu/ze_primitives.hpp"
#include "sched/entry/gpu/ze_cache.hpp"
#include "sched/entry/gpu/ze_a2a_allreduce_entry.hpp"
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
          cnt(cnt),
          dtype(dtype),
          op(op),
          buf_size_bytes(dtype.size() * cnt),
          peer_count(comm->size() - 1) {}

ze_a2a_allreduce_entry::~ze_a2a_allreduce_entry() {
    finalize();
}

void ze_a2a_allreduce_entry::kernel_init(size_t segment_count, size_t segment_size) {
    global_data::get().ze_cache->get(context, device, "kernels.spv", &module);
    std::string kernel_name =
        "reduce_local_inplace_kernel_" + to_string(dtype.idx()) + "_" + ccl_reduction_to_str(op);

    kernels.reserve(peer_count);
    for (int i = 1; i < peer_count; ++i) {
        void* input_buf = (char*)tmp_buf + i * segment_size;
        void* inoutput_buf = tmp_buf;
        kernels.emplace_back(module, kernel_name, worker_idx);
        kernels.back().set_args({ { sizeof(segment_count), &segment_count },
                                  { sizeof(input_buf), &input_buf },
                                  { sizeof(inoutput_buf), &inoutput_buf } });
        kernels.back().calculate_group_size(segment_count);
        kernel_events.emplace_back(ze_base_entry::create_event());
    }

    void* input_buf = send_buf.get_ptr();
    void* inoutput_buf = tmp_buf;
    kernels.emplace_back(module, kernel_name, worker_idx);
    kernels.back().set_args({ { sizeof(segment_count), &segment_count },
                              { sizeof(input_buf), &input_buf },
                              { sizeof(inoutput_buf), &inoutput_buf } });
    kernels.back().calculate_group_size(segment_count);
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
        int peer_rank = (ze_base_entry::comm_rank + i + 1) % comm->size();
        sched->get_memory().handle_manager.get(peer_rank, 0, peer_send_bufs[i], comm);
        CCL_THROW_IF_NOT(peer_send_bufs[i].get_ptr(), "null IPC buffer is received");
        sched->get_memory().handle_manager.get(peer_rank, 1, peer_recv_bufs[i], comm);
        CCL_THROW_IF_NOT(peer_recv_bufs[i].get_ptr(), "null IPC buffer is received");
    }

    size_t segment_count = cnt / ze_base_entry::comm_size;
    CCL_THROW_IF_NOT(cnt % ze_base_entry::comm_size == 0,
                     "buffer size must be divisible by the comm size");

    size_t segment_size = buf_size_bytes / ze_base_entry::comm_size;
    CCL_THROW_IF_NOT(buf_size_bytes % ze_base_entry::comm_size == 0,
                     "buffer size must be divisible by the comm size");
    CCL_THROW_IF_NOT(segment_size % dtype.size() == 0, "buffer size must be divisible by datatype");
    CCL_THROW_IF_NOT(segment_count * dtype.size() == segment_size);

    barrier_event = ze_base_entry::create_event();

    tmp_buf_size_bytes = peer_count * segment_count * dtype.size();

    /* alloc temp buffer */
    global_data::get().ze_cache->get(worker_idx,
                                     ze_base_entry::context,
                                     ze_base_entry::device,
                                     default_device_mem_alloc_desc,
                                     tmp_buf_size_bytes,
                                     0, /*alignment*/
                                     &tmp_buf);

    kernel_init(segment_count, segment_size);

    /* copy peer segments to temp buffer */
    pre_copy_events.reserve(peer_count + 1);
    for (int i = 0; i < peer_count; ++i) {
        pre_copy_events.emplace_back(ze_base_entry::create_event());
        void* src = (char*)peer_send_bufs[i].get_ptr() + ze_base_entry::comm_rank * segment_size;
        void* dst = (char*)tmp_buf + i * segment_size;
        ZE_CALL(zeCommandListAppendMemoryCopy,
                (ze_base_entry::get_copy_list(),
                 dst,
                 src,
                 segment_size,
                 pre_copy_events.back(),
                 0,
                 nullptr));
    }

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

    /* copy segments to peer buffers */
    post_copy_events.reserve(peer_count + 1);
    for (int i = 0; i < peer_count; ++i) {
        post_copy_events.emplace_back(ze_base_entry::create_event());
        void* src = tmp_buf;
        void* dst = (char*)peer_recv_bufs[i].get_ptr() + ze_base_entry::comm_rank * segment_size;
        ZE_CALL(zeCommandListAppendMemoryCopy,
                (ze_base_entry::get_copy_list(),
                 dst,
                 src,
                 segment_size,
                 post_copy_events.back(),
                 1,
                 &kernel_events.back()));
    }

    /* copy result to my buffer */
    post_copy_events.emplace_back(ze_base_entry::create_event());
    void* src = tmp_buf;
    void* dst = (char*)recv_buf.get_ptr() + ze_base_entry::comm_rank * segment_size;
    ZE_CALL(zeCommandListAppendMemoryCopy,
            (ze_base_entry::get_copy_list(),
             dst,
             src,
             segment_size,
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

    /* temp buffer memory */
    global_data::get().ze_cache->push(worker_idx,
                                      context,
                                      device,
                                      default_device_mem_alloc_desc,
                                      tmp_buf_size_bytes,
                                      0, /*alignment*/
                                      tmp_buf);

    kernels.clear();

    ze_base_entry::finalize();

    LOG_DEBUG("finalization complete");
}
