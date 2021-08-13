#include "common/stream/stream.hpp"
#include "sched/queue/queue.hpp"

#include "sched/entry/gpu/ze_base_entry.hpp"
#include "sched/entry/gpu/ze_cache.hpp"
#include "sched/entry/gpu/ze_call.hpp"
#include "ze_primitives.hpp"

#include <CL/sycl/backend/level_zero.hpp>

using namespace ccl;
using namespace ccl::ze;

ze_base_entry::ze_base_entry(ccl_sched* sched, uint32_t add_event_count)
        : sched_entry(sched),
          sched(sched),
          add_event_count(add_event_count) {
    CCL_THROW_IF_NOT(sched, "no sched");
    auto comm = sched->coll_param.comm;
    CCL_THROW_IF_NOT(comm, "no comm");
    rank = comm->rank();
    comm_size = comm->size();
}

void ze_base_entry::init() {
    if (is_initialized) {
        return;
    }
    worker_idx = sched->queue->get_idx();

    CCL_THROW_IF_NOT(sched->coll_param.stream, "null stream");

    LOG_DEBUG("getting a native stream");
    auto native_stream = sched->coll_param.stream->get_native_stream(worker_idx);
    if (native_stream->get_backend() != sycl::backend::level_zero) {
        CCL_THROW("unsupported sycl backend");
    }

    auto sycl_device = native_stream->get_device();
    device = sycl_device.template get_native<sycl::backend::level_zero>();

    auto sycl_context = native_stream->get_context();
    context = sycl_context.template get_native<sycl::backend::level_zero>();

    /* create queue */
    uint32_t num_queue_groups;
    get_num_queue_groups(device, &num_queue_groups);

    ze_queue_properties_t queue_props;
    get_queues_properties(device, num_queue_groups, &queue_props);

    uint32_t comp_ordinal, comp_queue_index;
    get_comp_queue_ordinal(device, queue_props, &comp_ordinal);
    get_queue_index(queue_props, comp_ordinal, rank, &comp_queue_index);

    comp_queue_desc = default_comp_queue_desc;
    comp_queue_desc.index = comp_queue_index;
    comp_queue_desc.ordinal = comp_ordinal;
    global_data::get().ze_cache->get(worker_idx, context, device, &comp_queue_desc, &comp_queue);
    LOG_DEBUG("get compute queue: { ordinal: ",
              comp_queue_desc.ordinal,
              ", index: ",
              comp_queue_desc.index,
              " }");

    /* create list */
    comp_list_desc = default_cmd_list_desc;
    comp_list_desc.commandQueueGroupOrdinal = comp_ordinal;
    global_data::get().ze_cache->get(worker_idx, context, device, &comp_list_desc, &comp_list);
    LOG_DEBUG("get compute list: { ordinal: ", comp_list_desc.commandQueueGroupOrdinal, " }");

    /* create event pool */
    event_pool_desc = default_event_pool_desc;
    event_pool_desc.count = 1 + add_event_count; // at least one event to track progress
    global_data::get().ze_cache->get(worker_idx, context, &event_pool_desc, &event_pool);
    LOG_DEBUG("get event pool: { max event count: ", event_pool_desc.count, " }");

    /* create event */
    ze_event_desc_t event_desc = default_event_desc;
    event_desc.signal = ZE_EVENT_SCOPE_FLAG_SUBDEVICE;
    event_desc.wait = ZE_EVENT_SCOPE_FLAG_SUBDEVICE;
    event_desc.index = 0;
    ZE_CALL(zeEventCreate, (event_pool, &event_desc, &entry_event));

    is_initialized = true;
}

void ze_base_entry::finalize() {
    if (!is_initialized) {
        return;
    }
    ZE_CALL(zeEventDestroy, (entry_event));

    /* event pool */
    global_data::get().ze_cache->push(worker_idx, context, &event_pool_desc, &event_pool);

    /* list */
    global_data::get().ze_cache->push(worker_idx, context, device, &comp_list_desc, &comp_list);

    /* queue */
    global_data::get().ze_cache->push(worker_idx, context, device, &comp_queue_desc, &comp_queue);

    is_initialized = false;
}

void ze_base_entry::start() {
    LOG_DEBUG("execute command list");

    CCL_THROW_IF_NOT(entry_event, "no entry event");
    ZE_CALL(zeEventHostReset, (entry_event));
    ZE_CALL(zeCommandQueueExecuteCommandLists, (comp_queue, 1, &comp_list, nullptr));

    if (((global_data::env().ze_serialize_mode & ze_call::serialize_mode::block)) != 0) {
        LOG_DEBUG("wait until command lists are executed");
        ZE_CALL(zeHostSynchronize, (comp_queue));
    }
}

void ze_base_entry::update() {
    ze_result_t query_status;

    if (global_data::env().kernel_debug == 0) {
        query_status = zeEventQueryStatus(entry_event);
    }
    else {
        query_status = zeHostSynchronize(comp_queue);
    }

    if (query_status == ZE_RESULT_SUCCESS) {
        LOG_DEBUG("command list complete");
        status = ccl_sched_entry_status_complete;
    }
    else if (query_status == ZE_RESULT_NOT_READY) {
        // just return in case if the kernel is not ready yet, will check again on the next iteration
        return;
    }
    else {
        CCL_THROW("error at zeEventQueryStatus");
    }
}
