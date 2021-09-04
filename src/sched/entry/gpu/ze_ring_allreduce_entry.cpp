#include "common/stream/stream.hpp"
#include "sched/entry/gpu/ze_primitives.hpp"
#include "sched/entry/gpu/ze_cache.hpp"
#include "sched/entry/gpu/ze_ring_allreduce_entry.hpp"
#include "sched/queue/queue.hpp"

#include <string>

using namespace ccl;
using namespace ccl::ze;

ze_ring_allreduce_entry::ze_ring_allreduce_entry(ccl_sched* sched,
                                                 ccl_buffer send_buf,
                                                 ccl_buffer recv_buf,
                                                 size_t cnt,
                                                 const ccl_datatype& dtype,
                                                 reduction op,
                                                 ccl_comm* comm)
        : ze_base_entry(sched, comm, (comm->size() - 1) * event_group_count),
          send_buf(send_buf),
          recv_buf(recv_buf),
          cnt(cnt),
          dtype(dtype),
          op(op),
          stage_iter_count(comm->size() - 1),
          total_iter_count(stage_iter_count * 2) {}

ze_ring_allreduce_entry::~ze_ring_allreduce_entry() {
    finalize();
}

void ze_ring_allreduce_entry::atl_ops_init() {
    left_peer = (comm_size + comm_rank - 1) % comm_size;
    right_peer = (comm_rank + 1) % comm_size;
    recv_tags.resize(total_iter_count);
    send_tags.resize(total_iter_count);
    sync_recv_flags.resize(total_iter_count, 0);
    sync_send_flags.resize(total_iter_count, { static_cast<char>(comm_rank) });

    for (int i = 0; i < total_iter_count; ++i) {
        send_tags[i] = comm->atl->tag->create(
            sched->get_comm_id(), right_peer, sched->sched_id, sched->get_op_id() + i);
        recv_tags[i] = comm->atl->tag->create(
            sched->get_comm_id(), comm_rank, sched->sched_id, sched->get_op_id() + i);
    }

    rs_sync_sent.resize(stage_iter_count, false);
    ag_sync_sent.resize(stage_iter_count, false);

    LOG_DEBUG("atl_ops_init completed");
}

void ze_ring_allreduce_entry::atl_ops_finalize() {}

void ze_ring_allreduce_entry::gpu_init() {
    rs_copy_signal_events.resize(stage_iter_count);
    rs_copy_wait_events.resize(stage_iter_count);
    rs_reduce_signal_events.resize(stage_iter_count);
    rs_reduce_wait_events.resize(stage_iter_count);
    ag_copy_signal_events.resize(stage_iter_count);
    ag_copy_wait_events.resize(stage_iter_count);

    rs_copy_started.resize(stage_iter_count, false);
    rs_reduce_started.resize(stage_iter_count, false);
    ag_copy_started.resize(stage_iter_count, false);

    global_data::get().ze_cache->get(context, device, "kernels.spv", &module);

    std::string kernel_name =
        "reduce_local_inplace_kernel_" + to_string(dtype.idx()) + "_" + ccl_reduction_to_str(op);
    kernels.reserve(stage_iter_count);

    for (int i = 0; i < stage_iter_count; ++i) {
        rs_copy_signal_events[i] = ze_base_entry::create_event();
        rs_copy_wait_events[i] = ze_base_entry::create_event();
        rs_reduce_signal_events[i] = ze_base_entry::create_event();
        rs_reduce_wait_events[i] = ze_base_entry::create_event();
        ag_copy_signal_events[i] = ze_base_entry::create_event();
        ag_copy_wait_events[i] = ze_base_entry::create_event();
        kernels.emplace_back(module, kernel_name, worker_idx);
    }
}

void ze_ring_allreduce_entry::gpu_finalize() {
    kernels.clear();
}

void ze_ring_allreduce_entry::recv_sync_flag(int idx) {
    auto buf = &sync_recv_flags[idx];
    auto bytes = sizeof(sync_recv_flags[idx]);
    auto src = comm->get_global_rank(left_peer);
    auto tag = recv_tags.at(idx);
    atl_req_t* req = &recv_reqs[idx];
    LOG_DEBUG("start recv: { src: ", src, ", tag: ", tag, ", bytes: ", bytes, "}");
    auto status = comm->atl->atl_ep_recv(sched->bin->get_atl_ep(), buf, bytes, src, tag, req);
    CCL_THROW_IF_NOT(status == ATL_STATUS_SUCCESS, "atl status: ", atl_status_to_str(status));
}

void ze_ring_allreduce_entry::send_sync_flag(int idx) {
    auto buf = &sync_send_flags[idx];
    auto bytes = sizeof(sync_send_flags[idx]);
    auto dst = comm->get_global_rank(right_peer);
    auto tag = send_tags.at(idx);
    atl_req_t* req = &send_reqs[idx];
    LOG_DEBUG("start send: { dst: ", dst, ", tag: ", tag, ", bytes: ", bytes, "}");
    auto status = comm->atl->atl_ep_send(sched->bin->get_atl_ep(), buf, bytes, dst, tag, req);
    CCL_THROW_IF_NOT(status == ATL_STATUS_SUCCESS, "atl status: ", atl_status_to_str(status));
}

bool ze_ring_allreduce_entry::check_atl_req(atl_req_t* req) {
    if (!req->is_completed) {
        auto status = comm->atl->atl_ep_check(sched->bin->get_atl_ep(), req);
        CCL_THROW_IF_NOT(status == ATL_STATUS_SUCCESS, "atl status: ", atl_status_to_str(status));
    }
    return req->is_completed;
}

void ze_ring_allreduce_entry::validate_sync_flags() {
    for (int i = 0; i < stage_iter_count; ++i) {
        int value = sync_send_flags[i];
        CCL_THROW_IF_NOT(value == comm_rank);
        value = sync_recv_flags[i];
        CCL_THROW_IF_NOT(value == left_peer);
    }
}

void ze_ring_allreduce_entry::init() {
    if (ze_base_entry::is_initialized) {
        return;
    }

    LOG_DEBUG("init");

    ze_base_entry::init(init_mode::compute | init_mode::copy);

    atl_ops_init();
    gpu_init();

    // create kernels
    ccl_buffer right_send_buf;
    ccl_buffer right_recv_buf;
    int peer_rank = (comm_rank + 1) % comm_size;
    sched->get_memory().handle_manager.get(peer_rank, 0, right_send_buf, comm);
    sched->get_memory().handle_manager.get(peer_rank, 1, right_recv_buf, comm);

    send_buf_ptr = send_buf.get_ptr();
    recv_buf_ptr = recv_buf.get_ptr();
    right_recv_buf_ptr = right_recv_buf.get_ptr();

    CCL_THROW_IF_NOT(send_buf != recv_buf, "in-place not supported yet");

    // reduce_scatter stage

    size_t main_block_count = cnt / comm_size;
    size_t dtype_size = dtype.size();
    int block_idx = comm_rank;

    for (int i = 0; i < stage_iter_count; ++i) {
        size_t block_count = main_block_count;
        if (block_idx == (comm_size - 1))
            block_count += cnt % comm_size;
        int copy_offset = main_block_count * dtype_size * block_idx;

        LOG_DEBUG("reduce_scatter: { my rank: ",
                  comm->rank(),
                  ", iter: ",
                  i,
                  ", copy_offset: ",
                  copy_offset,
                  ", block_count: ",
                  block_count,
                  " }");
        void* src = (char*)recv_buf_ptr + copy_offset;
        void* dst = (char*)right_recv_buf_ptr + copy_offset;
        if (i == 0) {
            src = (char*)send_buf_ptr + copy_offset;
        }

        ZE_CALL(zeCommandListAppendMemoryCopy,
                (ze_base_entry::get_copy_list(),
                 dst,
                 src,
                 block_count * dtype_size,
                 rs_copy_signal_events[i],
                 1,
                 &rs_copy_wait_events[i]));

        block_idx = (block_idx + comm_size - 1) % comm_size;
        block_count = main_block_count;
        if (block_idx == (comm_size - 1))
            block_count += cnt % comm_size;
        int kernel_offset = main_block_count * dtype_size * block_idx;

        LOG_DEBUG("reduce_scatter: { my rank: ",
                  comm->rank(),
                  ", iter: ",
                  i,
                  ", kernel_offset: ",
                  copy_offset,
                  ", block_count: ",
                  block_count,
                  " }");
        void* input_buf = (char*)send_buf_ptr + kernel_offset;
        void* output_buf = (char*)recv_buf_ptr + kernel_offset;

        ze_kernel_args_t kernel_args = { { sizeof(block_count), &block_count },
                                         { sizeof(input_buf), &input_buf },
                                         { sizeof(output_buf), &output_buf } };
        kernels[i].set_args(kernel_args);
        kernels[i].calculate_group_size(block_count);

        ZE_CALL(zeCommandListAppendLaunchKernel,
                (ze_base_entry::comp_primitives.list,
                 kernels[i].get_kernel(),
                 kernels[i].get_group_count(),
                 rs_reduce_signal_events[i],
                 1,
                 &rs_reduce_wait_events[i]));
    }

    // allgather stage

    for (int i = 0; i < stage_iter_count; ++i) {
        size_t block_count = main_block_count;
        if (block_idx == (comm_size - 1))
            block_count += cnt % comm_size;

        int copy_offset = main_block_count * dtype_size * block_idx;

        LOG_DEBUG("allgather: { my rank: ",
                  comm->rank(),
                  ", iter: ",
                  i,
                  ", copy offset: ",
                  copy_offset,
                  ", block_count: ",
                  block_count,
                  " }");
        void* src = (char*)recv_buf_ptr + copy_offset;
        void* dst = (char*)right_recv_buf_ptr + copy_offset;

        ZE_CALL(zeCommandListAppendMemoryCopy,
                (ze_base_entry::get_copy_list(),
                 dst,
                 src,
                 block_count * dtype_size,
                 ag_copy_signal_events[i],
                 1,
                 &ag_copy_wait_events[i]));

        block_idx = (block_idx + comm_size - 1) % comm_size;
    }

    ze_base_entry::close_lists();

    LOG_DEBUG("init completed");
}

void ze_ring_allreduce_entry::start() {
    init();

    reset_fields();

    for (int i = 0; i < total_iter_count; ++i) {
        recv_sync_flag(i);
    }

    ze_base_entry::start();

    for (size_t i = 0; i < send_reqs.size(); ++i) {
        CCL_THROW_IF_NOT(!send_reqs[i].is_completed);
    }

    for (int i = 0; i < stage_iter_count; ++i) {
        CCL_THROW_IF_NOT(!ze_base_entry::is_event_completed(rs_copy_signal_events[i]));
        CCL_THROW_IF_NOT(!ze_base_entry::is_event_completed(rs_copy_wait_events[i]));
        CCL_THROW_IF_NOT(!ze_base_entry::is_event_completed(rs_reduce_signal_events[i]));
        CCL_THROW_IF_NOT(!ze_base_entry::is_event_completed(rs_reduce_wait_events[i]));
        CCL_THROW_IF_NOT(!ze_base_entry::is_event_completed(ag_copy_signal_events[i]));
        CCL_THROW_IF_NOT(!ze_base_entry::is_event_completed(ag_copy_wait_events[i]));
    }

    status = ccl_sched_entry_status_started;
}

void ze_ring_allreduce_entry::update() {
    while (!is_rs_completed && (iter_idx < stage_iter_count)) {
        if (!rs_copy_started[iter_idx]) {
            ZE_CALL(zeEventHostSignal, (rs_copy_wait_events[iter_idx]));
            rs_copy_started[iter_idx] = true;
        }

        if (!rs_sync_sent[iter_idx] &&
            (ze_base_entry::is_event_completed(rs_copy_signal_events[iter_idx]))) {
            send_sync_flag(iter_idx);
            rs_sync_sent[iter_idx] = true;
        }

        if (!rs_reduce_started[iter_idx]) {
            auto recv_is_completed = check_atl_req(&recv_reqs[iter_idx]);
            if (recv_is_completed) {
                ZE_CALL(zeEventHostSignal, (rs_reduce_wait_events[iter_idx]));
                rs_reduce_started[iter_idx] = true;
            }
            else {
                return;
            }
        }

        if ((ze_base_entry::is_event_completed(rs_reduce_signal_events[iter_idx])) &&
            rs_sync_sent[iter_idx] && check_atl_req(&send_reqs[iter_idx])) {
            LOG_DEBUG("completed reduce_scatter iter ", iter_idx);
            iter_idx++;
        }
        else {
            return;
        }
    }
    is_rs_completed = true;

    iter_idx = 0;

    while (!is_ag_completed && (iter_idx < stage_iter_count)) {
        if (!ag_copy_started[iter_idx]) {
            ZE_CALL(zeEventHostSignal, (ag_copy_wait_events[iter_idx]));
            ag_copy_started[iter_idx] = true;
        }

        if (!ag_sync_sent[iter_idx] &&
            (ze_base_entry::is_event_completed(ag_copy_signal_events[iter_idx]))) {
            send_sync_flag(iter_idx + stage_iter_count);
            ag_sync_sent[iter_idx] = true;
        }

        auto is_send_completed =
            ag_sync_sent[iter_idx] && check_atl_req(&send_reqs[iter_idx + stage_iter_count]);
        auto is_recv_completed = check_atl_req(&recv_reqs[iter_idx + stage_iter_count]);
        if (is_send_completed && is_recv_completed) {
            LOG_DEBUG("completed allgatherv iter ", iter_idx);
            ++iter_idx;
        }
        else {
            return;
        }
    }

    is_ag_completed = true;

    validate_sync_flags();

    ZE_CALL(zeEventHostSignal, (ze_base_entry::entry_event));
    status = ccl_sched_entry_status_complete;

    if (status == ccl_sched_entry_status_complete && !sched->coll_attr.to_cache) {
        finalize();
    }
}

void ze_ring_allreduce_entry::finalize() {
    if (!ze_base_entry::is_initialized) {
        return;
    }

    LOG_DEBUG("finalization");

    gpu_finalize();
    atl_ops_finalize();

    ze_base_entry::finalize();

    LOG_DEBUG("finalization complete");
}

void ze_ring_allreduce_entry::reset_fields() {
    iter_idx = 0;
    is_rs_completed = is_ag_completed = false;

    send_reqs.clear();
    send_reqs.resize(total_iter_count);
    recv_reqs.clear();
    recv_reqs.resize(total_iter_count);

    std::fill(sync_recv_flags.begin(), sync_recv_flags.end(), 0);

    std::fill(rs_sync_sent.begin(), rs_sync_sent.end(), false);
    std::fill(ag_sync_sent.begin(), ag_sync_sent.end(), false);

    std::fill(rs_copy_started.begin(), rs_copy_started.end(), false);
    std::fill(rs_reduce_started.begin(), rs_reduce_started.end(), false);
    std::fill(ag_copy_started.begin(), ag_copy_started.end(), false);
}
