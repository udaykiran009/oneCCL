#include "common/comm/l0/modules/kernel_utils.hpp"
#include "common/stream/stream.hpp"
#include "sched/queue/queue.hpp"
#include "ze_primitives.hpp"
#include "ze_cache.hpp"
#include "ze_allreduce_entry.hpp"
#include <CL/sycl/backend/level_zero.hpp>
#include <string>

using namespace ccl;
using namespace ccl::ze;

ze_allreduce_entry::ze_allreduce_entry(ccl_sched* sched,
                                       ccl_buffer send_buf,
                                       ccl_buffer recv_buf,
                                       size_t cnt,
                                       const ccl_datatype& dtype,
                                       reduction op,
                                       ccl_comm* comm)
        : sched_entry(sched),
          sched(sched),
          send_buf(send_buf),
          recv_buf(recv_buf),
          rank(comm->rank()),
          comm_size(comm->size()),
          cnt(cnt),
          dtype(dtype),
          op(op),
          buf_size_bytes(dtype.size() * cnt) {}

void ze_allreduce_entry::init() {
    LOG_DEBUG(name(), " entry: initialization");

    // TODO: can we to check that ze is allready initialized?
    ze_init::instance();

    ze_driver_handle_t driver;

    if (sched && sched->coll_param.stream) {
        LOG_DEBUG(name(), " entry: getting a native stream");
        auto native_stream = sched->coll_param.stream->get_native_stream(sched->queue->get_idx());
        if (native_stream->get_backend() != cl::sycl::backend::level_zero) {
            CCL_THROW(name(), " entry: unsupported sycl backend");
        }

        auto sycl_device = native_stream->get_device();
        device = sycl_device.template get_native<cl::sycl::backend::level_zero>();

        auto sycl_context = native_stream->get_context();
        context = sycl_context.template get_native<cl::sycl::backend::level_zero>();

        auto sycl_platform = sycl_context.get_platform();
        driver = sycl_platform.template get_native<cl::sycl::backend::level_zero>();
    }
    else {
        CCL_THROW(name(), " entry: algo without stream is unsupported");
    }

    uint32_t num_queue_groups;
    get_num_queue_groups(device, &num_queue_groups);

    ze_queue_properties_t queue_props;
    get_queues_properties(device, num_queue_groups, &queue_props);

    uint32_t comp_ordinal, comp_queue_index;
    get_comp_queue_ordinal(device, queue_props, &comp_ordinal);
    get_queue_index(queue_props, comp_ordinal, rank, &comp_queue_index);

    ze_command_queue_desc_t comp_queue_desc = default_comp_queue_desc;
    comp_queue_desc.index = comp_queue_index;
    comp_queue_desc.ordinal = comp_ordinal;
    ZE_CALL(zeCommandQueueCreate(context, device, &comp_queue_desc, &comp_queue));

    ze_command_list_desc_t comp_list_desc = default_cmd_list_desc;
    comp_list_desc.commandQueueGroupOrdinal = comp_ordinal;
    ZE_CALL(zeCommandListCreate(
        context, device, &comp_list_desc, &comp_list)); // TODO: we can cache it?

    ze_module_loader<ccl_coll_allreduce>::instance().get(device, context, &module);

    std::string kernel_name =
        "allreduce_execution_" + to_string(dtype.idx()) + "_" + ccl_reduction_to_str(op);
    create_kernel(module, kernel_name, &kernel); // TODO: we can cache it

    ze_group_size_t group_size;
    get_suggested_group_size(kernel, cnt, &group_size);
    LOG_DEBUG(name(), " entry: suggested group size: ", to_string(group_size));

    get_suggested_group_count(group_size, cnt, &group_count);
    LOG_DEBUG(name(), " entry: suggested group count: ", to_string(group_count));

    ZE_CALL(zeKernelSetGroupSize(
        kernel, group_size.groupSizeX, group_size.groupSizeY, group_size.groupSizeZ));

    ccl_buffer right_send_buf;
    ccl_buffer right_recv_buf;
    int peer_rank = 1;
    sched->get_ccl_sched_memory().handle_manager.get(peer_rank, 0, right_send_buf);
    sched->get_ccl_sched_memory().handle_manager.get(peer_rank, 1, right_recv_buf);
    LOG_DEBUG(name(),
              " entry: get IPC pointers from ",
              peer_rank,
              " rank: right_send_buf: ",
              right_send_buf,
              ", right_recv_buf: ",
              right_recv_buf);

    send_buf_ptr = send_buf.get_ptr();
    recv_buf_ptr = recv_buf.get_ptr();
    right_send_buf_ptr = right_send_buf.get_ptr();
    right_recv_buf_ptr = right_recv_buf.get_ptr();
    ze_kernel_args_t kernel_args = { { sizeof(rank), &rank },
                                     { sizeof(comm_size), &comm_size },
                                     { sizeof(cnt), &cnt },
                                     { sizeof(send_buf_ptr), &send_buf_ptr },
                                     { sizeof(recv_buf_ptr), &recv_buf_ptr },
                                     { sizeof(right_send_buf_ptr), &right_send_buf_ptr },
                                     { sizeof(right_recv_buf_ptr), &right_recv_buf_ptr } };

    LOG_DEBUG(name(), " entry: kernel args:\n", to_string(kernel_args));
    set_kernel_args(kernel, kernel_args);

    ZE_CALL(zeCommandListAppendLaunchKernel(comp_list, kernel, &group_count, nullptr, 0, nullptr));
    ZE_CALL(zeCommandListClose(comp_list));

    ze_fence_desc_t fence_desc = default_fence_desc;
    ZE_CALL(zeFenceCreate(comp_queue, &fence_desc, &fence)); // TODO: we can cache it

    LOG_DEBUG(name(), " entry: initialization complete");
}

void ze_allreduce_entry::start() {
    init();
    LOG_DEBUG(name(), " entry: execute command list");
    ZE_CALL(zeCommandQueueExecuteCommandLists(comp_queue, 1, &comp_list, fence));

    status = ccl_sched_entry_status_started;
}

void ze_allreduce_entry::update() {
    ze_result_t fence_status;
    if (global_data::env().comm_kernels_debug == 0) {
        fence_status = zeFenceQueryStatus(fence);
    }
    else {
        fence_status = zeCommandQueueSynchronize(comp_queue, std::numeric_limits<uint64_t>::max());
    }

    if (fence_status == ZE_RESULT_SUCCESS) {
        LOG_DEBUG(name(), " entry: command list complete");
        finalize();
        status = ccl_sched_entry_status_complete;
    }
    else if (fence_status == ZE_RESULT_NOT_READY) {
        // just return in case if the kernel is not ready yet, will check again on the next iteration
        return;
    }
    else {
        CCL_THROW(name(), " entry: error at zeFenceQueryStatus");
    }
}

void ze_allreduce_entry::finalize() {
    LOG_DEBUG(name(), " entry: finalization");
    ZE_CALL(zeCommandListReset(comp_list));
    ZE_CALL(zeFenceDestroy(fence));
    ZE_CALL(zeCommandQueueDestroy(comp_queue));
    ZE_CALL(zeCommandListDestroy(comp_list));
    ZE_CALL(zeKernelDestroy(kernel));
    LOG_DEBUG(name(), " entry: finalization complete");
}
