#include "reduce_local_entry.hpp"

#if defined(CCL_ENABLE_SYCL) && defined(MULTI_GPU_SUPPORT)
#include "coll/coll_check.hpp"
#include "common/comm/l0/modules/kernel_utils.hpp"
#include "common/datatype/datatype.hpp"
#include "common/stream/stream.hpp"
#include "gpu/ze_primitives.hpp"
#include "gpu/ze_cache.hpp"
#include "sched/queue/queue.hpp"

#include <string>

using namespace ccl;
using namespace ccl::ze;

void reduce_local_entry::init() {
    if (is_initialized) {
        return;
    }

    LOG_DEBUG("initialization");

    CCL_THROW_IF_NOT(sched != nullptr, "no sched");
    worker_idx = sched->queue->get_idx();

    CCL_THROW_IF_NOT(sched->coll_param.stream, "null stream in ze_reduce_local_entry");

    LOG_DEBUG("getting a native stream");
    auto native_stream = sched->coll_param.stream->get_native_stream(worker_idx);
    if (native_stream->get_backend() != sycl::backend::level_zero) {
        CCL_THROW("unsupported sycl backend");
    }

    auto sycl_device = native_stream->get_device();
    device = sycl_device.template get_native<sycl::backend::level_zero>();

    auto sycl_context = native_stream->get_context();
    context = sycl_context.template get_native<sycl::backend::level_zero>();

    uint32_t num_queue_groups;
    get_num_queue_groups(device, &num_queue_groups);

    ze_queue_properties_t queue_props;
    get_queues_properties(device, num_queue_groups, &queue_props);

    uint32_t comp_ordinal, comp_queue_index;
    get_comp_queue_ordinal(device, queue_props, &comp_ordinal);
    get_queue_index(queue_props, comp_ordinal, 0, &comp_queue_index);

    comp_queue_desc = default_comp_queue_desc;
    comp_queue_desc.index = comp_queue_index;
    comp_queue_desc.ordinal = comp_ordinal;

    ccl::global_data::get().ze_cache->get(
        worker_idx, context, device, &comp_queue_desc, &comp_queue);
    LOG_DEBUG("get compute queue: { ordinal: ",
              comp_queue_desc.ordinal,
              ", index: ",
              comp_queue_desc.index,
              " }");

    comp_list_desc = default_cmd_list_desc;
    comp_list_desc.commandQueueGroupOrdinal = comp_ordinal;
    ccl::global_data::get().ze_cache->get(worker_idx, context, device, &comp_list_desc, &comp_list);
    LOG_DEBUG("get compute list: { ordinal: ", comp_list_desc.commandQueueGroupOrdinal, " }");

    ccl::global_data::get().ze_cache->get(context, device, &module, "kernels.spv");

    kernel_name =
        "reduce_local_inplace_kernel_" + to_string(dtype.idx()) + "_" + ccl_reduction_to_str(op);
    ccl::global_data::get().ze_cache->get(worker_idx, module, kernel_name, &kernel);
    LOG_DEBUG("get kernel: name: ", kernel_name);

    ze_group_size_t group_size;
    get_suggested_group_size(kernel, in_cnt, &group_size);
    LOG_DEBUG("suggested group size: ", to_string(group_size));

    get_suggested_group_count(group_size, in_cnt, &group_count);
    LOG_DEBUG("suggested group count: ", to_string(group_count));

    ZE_CALL(zeKernelSetGroupSize,
            (kernel, group_size.groupSizeX, group_size.groupSizeY, group_size.groupSizeZ));

    size_t bytes = in_cnt * dtype.size();
    in_buf_ptr = in_buf.get_ptr(bytes);
    inout_buf_ptr = inout_buf.get_ptr(bytes);
    ze_kernel_args_t kernel_args = { { sizeof(in_cnt), &in_cnt },
                                     { sizeof(in_buf_ptr), &in_buf_ptr },
                                     { sizeof(inout_buf_ptr), &inout_buf_ptr } };

    LOG_DEBUG("kernel ", kernel, " args:\n", to_string(kernel_args));
    set_kernel_args(kernel, kernel_args);

    ZE_CALL(zeCommandListAppendLaunchKernel,
            (comp_list, kernel, &group_count, nullptr, 0, nullptr));
    ZE_CALL(zeCommandListClose, (comp_list));

    fence_desc = default_fence_desc;
    ccl::global_data::get().ze_cache->get(worker_idx, comp_queue, &fence_desc, &fence);

    LOG_DEBUG("initialization complete");
}

void reduce_local_entry::update() {
    CCL_THROW_IF_NOT(use_device);

    ze_result_t query_status;
    if (ccl::global_data::env().kernel_debug == 0) {
        query_status = zeFenceQueryStatus(fence);
    }
    else {
        query_status = zeCommandQueueSynchronize(comp_queue, std::numeric_limits<uint64_t>::max());
    }

    if (query_status == ZE_RESULT_SUCCESS) {
        LOG_DEBUG("command list complete");
        if (!sched->coll_attr.to_cache) {
            finalize();
        }
        status = ccl_sched_entry_status_complete;
    }
    else if (query_status == ZE_RESULT_NOT_READY) {
        // just return in case if the kernel is not ready yet, will check again on the next iteration
        return;
    }
    else {
        CCL_THROW("error at zeFenceQueryStatus");
    }

    if (ccl::global_data::get().kernel_counter > 0) {
        std::atomic_fetch_sub<size_t>(&ccl::global_data::get().kernel_counter, 1);
    }
}

void reduce_local_entry::check_use_device() {
    use_device = false;
    ccl_stream* stream = (ccl_stream*)sched->coll_param.stream;
    if (fn || !stream)
        return;

    size_t bytes = in_cnt * dtype.size();
    sycl::queue* q = stream->get_native_stream(sched->queue->get_idx());
    CCL_THROW_IF_NOT(q, "null sycl queue");
    auto in_ptr_type = sycl::get_pointer_type(in_buf.get_ptr(bytes), q->get_context());
    auto inout_ptr_type = sycl::get_pointer_type(inout_buf.get_ptr(bytes), q->get_context());

    LOG_DEBUG("in_ptr_type: ",
              ccl_usm_type_to_str(in_ptr_type),
              ", inout_ptr_type: ",
              ccl_usm_type_to_str(inout_ptr_type),
              ", native_stream: ",
              stream->to_string(),
              ", in_count: ",
              in_cnt)

    if ((in_ptr_type == sycl::usm::alloc::device) && (inout_ptr_type == sycl::usm::alloc::device)) {
        use_device = true;
    }
}

void reduce_local_entry::start_on_device() {
    init();

    if (is_initialized && status == ccl_sched_entry_status_not_started) {
        ZE_CALL(zeFenceReset, (fence));
    }

    size_t kernel_counter = 0;
    if (ccl::global_data::env().enable_kernel_sync) {
        kernel_counter = std::atomic_fetch_add<size_t>(&ccl::global_data::get().kernel_counter, 1);
    }

    if (kernel_counter == 0) {
        LOG_DEBUG("execute command list");
        ZE_CALL(zeCommandQueueExecuteCommandLists, (comp_queue, 1, &comp_list, fence));
        status = ccl_sched_entry_status_started;
    }
    else {
        std::atomic_fetch_sub<size_t>(&ccl::global_data::get().kernel_counter, 1);
        status = ccl_sched_entry_status_again;
    }
}

void reduce_local_entry::finalize() {
    if (!is_initialized) {
        return;
    }

    LOG_DEBUG("finalization");

    // fence_cache
    ccl::global_data::get().ze_cache->push(worker_idx, comp_queue, &fence_desc, &fence);
    // module_cache
    ccl::global_data::get().ze_cache->push(worker_idx, module, kernel_name, &kernel);
    // list_cache
    ccl::global_data::get().ze_cache->push(
        worker_idx, context, device, &comp_list_desc, &comp_list);
    // queue_cache
    ccl::global_data::get().ze_cache->push(
        worker_idx, context, device, &comp_queue_desc, &comp_queue);

    is_initialized = false;

    LOG_DEBUG("finalization complete");
}
#endif // defined(CCL_ENABLE_SYCL) && defined(MULTI_GPU_SUPPORT)
