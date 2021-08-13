#include "common/comm/l0/modules/kernel_utils.hpp"
#include "common/stream/stream.hpp"
#include "sched/entry/gpu/ze_cache.hpp"
#include "sched/entry/gpu/ze_primitives.hpp"
#include "sched/entry/gpu/ze_reduce_entry.hpp"
#include "sched/queue/queue.hpp"

#include <string>

using namespace ccl;
using namespace ccl::ze;

ze_reduce_entry::ze_reduce_entry(ccl_sched* sched,
                                 ccl_buffer send_buf,
                                 ccl_buffer recv_buf,
                                 size_t cnt,
                                 const ccl_datatype& dtype,
                                 reduction op,
                                 int root,
                                 ccl_comm* comm)
        : ze_base_entry(sched, 2 /* request additional events */),
          send_buf(send_buf),
          recv_buf(recv_buf),
          cnt(cnt),
          dtype(dtype),
          op(op),
          root(root),
          comm(comm),
          buf_size_bytes(dtype.size() * cnt),
          is_initialized(false),
          empty_kernel_event(nullptr),
          empty_kernel(nullptr),
          empty_kernel_name("empty_kernel") {}

ze_reduce_entry::~ze_reduce_entry() {
    finalize();
}

void ze_reduce_entry::init() {
    if (is_initialized) {
        return;
    }

    LOG_DEBUG("initialization");

    ze_base_entry::init();

    /* create kernels */
    ccl_buffer right_send_buf;
    int peer_rank = (rank + 1) % comm_size;
    if (comm->rank() == root) {
        sched->get_memory().handle_manager.get(peer_rank, 0, right_send_buf);
    }

    LOG_DEBUG(
        "get IPC pointers from ", peer_rank, " by ", root, ", right_send_buf: ", right_send_buf);

    send_buf_ptr = send_buf.get_ptr();
    recv_buf_ptr = recv_buf.get_ptr();

    right_send_buf_ptr = right_send_buf.get_ptr();

    ze_kernel_args_t reduce_local_kernel_args = { { sizeof(rank), &rank },
                                                  { sizeof(comm_size), &comm_size },
                                                  { sizeof(cnt), &cnt },
                                                  { sizeof(send_buf_ptr), &send_buf_ptr },
                                                  { sizeof(tmp_buf_ptr), &tmp_buf_ptr },
                                                  { sizeof(recv_buf_ptr), &recv_buf_ptr } };

    ccl::global_data::get().ze_cache->get(context, device, &module, "kernels.spv");

    main_kernel_name = "reduce_local_outofplace_kernel_";
    device_mem_alloc_desc = default_device_mem_alloc_desc;
    ccl::global_data::get().ze_cache->get(worker_idx,
                                          context,
                                          device,
                                          &device_mem_alloc_desc,
                                          buf_size_bytes,
                                          0, /*alignment*/
                                          &tmp_buf_ptr);

    main_kernel_name += to_string(dtype.idx()) + "_" + ccl_reduction_to_str(op);
    LOG_DEBUG("get kernel: name: ", main_kernel_name);
    ccl::global_data::get().ze_cache->get(worker_idx, module, main_kernel_name, &main_kernel);

    auto& main_kernel_args = reduce_local_kernel_args;
    LOG_DEBUG("kernel ", main_kernel, " args:\n", to_string(main_kernel_args));
    set_kernel_args(main_kernel, main_kernel_args);

    ze_group_size_t group_size;
    get_suggested_group_size(main_kernel, cnt, &group_size);
    LOG_DEBUG("suggested group size: ", to_string(group_size));

    get_suggested_group_count(group_size, cnt, &group_count);
    LOG_DEBUG("suggested group count: ", to_string(group_count));

    ZE_CALL(zeKernelSetGroupSize,
            (main_kernel, group_size.groupSizeX, group_size.groupSizeY, group_size.groupSizeZ));

    if (ccl::global_data::env().enable_kernel_1s_ipc_wa) {
        LOG_DEBUG("get kernel: name: ", empty_kernel_name);
        ccl::global_data::get().ze_cache->get(worker_idx, module, empty_kernel_name, &empty_kernel);
        CCL_THROW_IF_NOT(empty_kernel, "null empty_kernel");
        /* use allreduce_kernel_args since they have pointers to peer mem */
        set_kernel_args(empty_kernel, main_kernel_args);
    }

    ze_event_desc_t event_desc = default_event_desc;
    event_desc.signal = ZE_EVENT_SCOPE_FLAG_SUBDEVICE;
    event_desc.wait = ZE_EVENT_SCOPE_FLAG_SUBDEVICE;

    uint32_t last_event_idx = 1; // 0 is used to track entry progress

    if (empty_kernel) {
        LOG_DEBUG("create event for empty kernel");
        event_desc.index = last_event_idx++;
        ZE_CALL(zeEventCreate, (event_pool, &event_desc, &empty_kernel_event));
    }

    event_desc.index = last_event_idx++;
    ZE_CALL(zeEventCreate, (event_pool, &event_desc, &copy_from_peer_event));

    LOG_DEBUG("real event count: ", last_event_idx);

    /* do appends */
    if (empty_kernel) {
        LOG_DEBUG("append empty kernel");
        ze_group_count_t empty_group_count = { 1, 1, 1 };
        ZE_CALL(zeCommandListAppendLaunchKernel,
                (comp_list, empty_kernel, &empty_group_count, empty_kernel_event, 0, nullptr));
    }

    LOG_DEBUG("one-sided multi-phase algorithm");

    ZE_CALL(zeCommandListAppendMemoryCopy,
            (comp_list,
             tmp_buf_ptr,
             right_send_buf_ptr,
             buf_size_bytes,
             copy_from_peer_event,
             (empty_kernel_event) ? 1 : 0,
             &empty_kernel_event));

    ZE_CALL(zeCommandListAppendLaunchKernel,
            (comp_list, main_kernel, &group_count, entry_event, 1, &copy_from_peer_event));

    ZE_CALL(zeCommandListClose, (comp_list));

    is_initialized = true;

    LOG_DEBUG("initialization complete");
}

void ze_reduce_entry::start() {
    init();

    if (is_initialized && status == ccl_sched_entry_status_not_started) {
        reset_sync_objects();
    }

    size_t kernel_counter = 0;
    if (ccl::global_data::env().enable_kernel_sync) {
        kernel_counter = ccl::global_data::get().kernel_counter++;
    }

    if (kernel_counter == 0) {
        ze_base_entry::start();
        status = ccl_sched_entry_status_started;
    }
    else {
        ccl::global_data::get().kernel_counter--;
        status = ccl_sched_entry_status_again;
    }
}

void ze_reduce_entry::update() {
    ze_base_entry::update();
    if (status == ccl_sched_entry_status_complete && !sched->coll_attr.to_cache) {
        finalize();
    }

    if (ccl::global_data::env().enable_kernel_sync && ccl::global_data::get().kernel_counter > 0) {
        ccl::global_data::get().kernel_counter--;
    }
}

void ze_reduce_entry::finalize() {
    if (!is_initialized) {
        return;
    }

    LOG_DEBUG("finalization");

    /* events */
    LOG_DEBUG("copy ops finalization");
    ZE_CALL(zeEventDestroy, (copy_from_peer_event));
    /* device mem */
    ccl::global_data::get().ze_cache->push(worker_idx,
                                           context,
                                           device,
                                           &device_mem_alloc_desc,
                                           buf_size_bytes,
                                           0, /*alignment*/
                                           &tmp_buf_ptr);

    /* kernels */
    if (empty_kernel_event) {
        ZE_CALL(zeEventDestroy, (empty_kernel_event));
        ccl::global_data::get().ze_cache->push(
            worker_idx, module, empty_kernel_name, &empty_kernel);
    }
    ccl::global_data::get().ze_cache->push(worker_idx, module, main_kernel_name, &main_kernel);

    ze_base_entry::finalize();

    is_initialized = false;

    LOG_DEBUG("finalization complete");
}

void ze_reduce_entry::reset_sync_objects() {
    if (empty_kernel_event) {
        ZE_CALL(zeEventHostReset, (empty_kernel_event));
    }
    ZE_CALL(zeEventHostReset, (copy_from_peer_event));
}
