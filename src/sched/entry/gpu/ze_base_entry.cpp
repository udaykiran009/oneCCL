#include "common/stream/stream.hpp"
#include "sched/queue/queue.hpp"

#include "sched/entry/gpu/ze_base_entry.hpp"
#include "sched/entry/gpu/ze_cache.hpp"
#include "sched/entry/gpu/ze_call.hpp"
#include "ze_primitives.hpp"

#include <CL/sycl/backend/level_zero.hpp>

using namespace ccl;
using namespace ccl::ze;

ze_base_entry::ze_base_entry(ccl_sched *sched, ccl_comm *comm, uint32_t add_event_count)
        : sched_entry(sched),
          sched(sched),
          comm(comm) {
    CCL_THROW_IF_NOT(sched, "no sched");
    if (!comm) {
        comm = sched->coll_param.comm;
    }
    CCL_THROW_IF_NOT(comm, "no comm");
    comm_rank = comm->rank();
    comm_size = comm->size();
    events.resize(add_event_count + 1, nullptr); // at least one event to track progress
}

void ze_base_entry::init(init_mode ze_init_mode) {
    if (is_initialized) {
        return;
    }
    worker_idx = sched->queue->get_idx();

    CCL_THROW_IF_NOT(sched->coll_param.stream, "null stream");

    LOG_DEBUG("getting a native stream");
    auto native_stream = sched->coll_param.stream->get_native_stream(worker_idx);
    if (native_stream->get_backend() != sycl::backend::level_zero) {
        CCL_THROW("unsupported SYCL backend");
    }

    auto sycl_device = native_stream->get_device();
    device = sycl_device.template get_native<sycl::backend::level_zero>();

    auto sycl_context = native_stream->get_context();
    context = sycl_context.template get_native<sycl::backend::level_zero>();

    /* get queue properties */
    uint32_t num_queue_groups;
    get_num_queue_groups(device, &num_queue_groups);

    ze_queue_properties_t queue_props;
    get_queues_properties(device, num_queue_groups, &queue_props);

    /* init compute queue, list */
    if (init_mode::compute & ze_init_mode) {
        LOG_DEBUG("compute init mode is enabled");
        get_comp_primitives(queue_props, comp_primitives);
        init_primitives(comp_primitives);
    }

    /* init copy queue, list */
    if (init_mode::copy & ze_init_mode) {
        LOG_DEBUG("copy init mode is enabled");
        get_copy_primitives(queue_props, copy_primitives, ze_init_mode);
        init_primitives(copy_primitives);
    }

    /* create event pool */
    event_pool_desc = default_event_pool_desc;
    event_pool_desc.count = events.size();
    global_data::get().ze_cache->get(worker_idx, context, event_pool_desc, &event_pool);
    LOG_DEBUG("get event pool: { max event count: ", event_pool_desc.count, " }");

    entry_event = create_event();

    is_initialized = true;
}

void ze_base_entry::finalize() {
    if (!is_initialized) {
        return;
    }

    destroy_events();

    /* event pool */
    global_data::get().ze_cache->push(worker_idx, context, event_pool_desc, event_pool);

    if (comp_primitives.list && comp_primitives.queue) {
        LOG_DEBUG("push from cache for compute list and queue");
        /* list */
        global_data::get().ze_cache->push(
            worker_idx, context, device, comp_primitives.list_desc, comp_primitives.list);

        /* queue */
        global_data::get().ze_cache->push(
            worker_idx, context, device, comp_primitives.queue_desc, comp_primitives.queue);
    }

    if (copy_primitives.list && copy_primitives.queue) {
        LOG_DEBUG("push from cache for copy list and queue");
        /* copy list */
        global_data::get().ze_cache->push(
            worker_idx, context, device, copy_primitives.list_desc, copy_primitives.list);

        /* copy queue */
        global_data::get().ze_cache->push(
            worker_idx, context, device, copy_primitives.queue_desc, copy_primitives.queue);
    }

    is_initialized = false;
}

void ze_base_entry::start() {
    reset_events();

    if (comp_primitives.list && comp_primitives.queue) {
        LOG_DEBUG("execute compute command list");
        ZE_CALL(zeCommandQueueExecuteCommandLists,
                (comp_primitives.queue, 1, &comp_primitives.list, nullptr));
    }

    if (copy_primitives.list && copy_primitives.queue) {
        LOG_DEBUG("execute copy command list");
        ZE_CALL(zeCommandQueueExecuteCommandLists,
                (copy_primitives.queue, 1, &copy_primitives.list, nullptr));
    }

    if (((global_data::env().ze_serialize_mode & ze_call::serialize_mode::block)) != 0) {
        LOG_DEBUG("wait until command lists are executed");
        if (copy_primitives.queue)
            ZE_CALL(zeHostSynchronize, (copy_primitives.queue));
        if (comp_primitives.queue)
            ZE_CALL(zeHostSynchronize, (comp_primitives.queue));
    }
}

bool ze_base_entry::is_event_completed(ze_event_handle_t event) {
    ze_result_t res = zeEventQueryStatus(event);
    CCL_THROW_IF_NOT(res == ZE_RESULT_SUCCESS || res == ZE_RESULT_NOT_READY,
                     "unexpected result from zeEventQueryStatus: ",
                     to_string(res));
    return (res == ZE_RESULT_SUCCESS);
}

void ze_base_entry::update() {
    ze_result_t query_status;

    if (global_data::env().kernel_debug == 0) {
        query_status = zeEventQueryStatus(entry_event);
    }
    else {
        if (copy_primitives.queue)
            query_status = zeHostSynchronize(copy_primitives.queue);
        if (comp_primitives.queue)
            query_status = zeHostSynchronize(comp_primitives.queue);
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

ze_command_list_handle_t ze_base_entry::get_copy_list() {
    ze_command_list_handle_t list = nullptr;
    if (copy_primitives.list) {
        list = copy_primitives.list;
        LOG_DEBUG("copy list is returned");
    }
    else {
        list = comp_primitives.list;
        LOG_DEBUG("compute list is returned");
    }
    CCL_THROW_IF_NOT(list, "command list is invalid");
    return list;
}

void ze_base_entry::get_comp_primitives(const ze_queue_properties_t &queue_props,
                                        cmd_primitives &comp_primitives) {
    uint32_t ordinal, queue_index;
    get_comp_queue_ordinal(device, queue_props, &ordinal);
    get_queue_index(queue_props, ordinal, 0, &queue_index);

    comp_primitives.queue_desc.ordinal = ordinal;
    comp_primitives.queue_desc.index = queue_index;
    comp_primitives.list_desc.commandQueueGroupOrdinal = ordinal;
}

void ze_base_entry::get_copy_primitives(const ze_queue_properties_t &queue_props,
                                        cmd_primitives &copy_primitives,
                                        init_mode ze_init_mode) {
    uint32_t ordinal, queue_index;
    get_copy_queue_ordinal(device, queue_props, &ordinal);

    // TODO: index depends on rank's changing, when > 1 queues are created,
    // the index is still the same for different queues, that's the issue.
    // WA is adding optional counter, which says the order number of a queue.
    // Need to think, how we'd calculate the index for every queue.
    // Hang in case of CCL_KERNEL_1S_USE_COPY_OPS=1 CCL_ZE_COPY_ENGINE=none
    if (ze_init_mode == (init_mode::compute | init_mode::copy)) {
        get_queue_index(queue_props, ordinal, 1, &queue_index);
    }
    else {
        get_queue_index(queue_props, ordinal, 0, &queue_index);
    }

    copy_primitives.queue_desc.ordinal = ordinal;
    copy_primitives.queue_desc.index = queue_index;
    copy_primitives.list_desc.commandQueueGroupOrdinal = ordinal;
}

void ze_base_entry::init_primitives(cmd_primitives &cmd_primitives) {
    global_data::get().ze_cache->get(
        worker_idx, context, device, cmd_primitives.queue_desc, &cmd_primitives.queue);
    LOG_DEBUG("get queue: { ordinal: ",
              cmd_primitives.queue_desc.ordinal,
              ", index: ",
              cmd_primitives.queue_desc.index,
              " }");

    global_data::get().ze_cache->get(
        worker_idx, context, device, cmd_primitives.list_desc, &cmd_primitives.list);
    LOG_DEBUG("get list: { ordinal: ", cmd_primitives.list_desc.commandQueueGroupOrdinal, " }");
}

ze_event_handle_t ze_base_entry::create_event() {
    ze_event_desc_t event_desc = default_event_desc;
    event_desc.signal = ZE_EVENT_SCOPE_FLAG_DEVICE;
    event_desc.wait = ZE_EVENT_SCOPE_FLAG_DEVICE;
    event_desc.index = event_counter++;
    LOG_DEBUG("create event with index ", event_desc.index);
    CCL_THROW_IF_NOT(event_desc.index < event_pool_desc.count, "event creation limit exceeded");
    CCL_THROW_IF_NOT(event_desc.index < events.size());

    ze_event_handle_t event;
    ZE_CALL(zeEventCreate, (event_pool, &event_desc, &event));
    events[event_desc.index] = event;

    return event;
}

void ze_base_entry::reset_events() {
    CCL_THROW_IF_NOT(entry_event, "no entry event");
    for (size_t idx = 0; idx < events.size(); idx++) {
        if (events[idx])
            ZE_CALL(zeEventHostReset, (events[idx]));
    }
}

void ze_base_entry::destroy_events() {
    CCL_THROW_IF_NOT(entry_event, "no entry event");
    for (size_t idx = 0; idx < events.size(); idx++) {
        if (events[idx])
            ZE_CALL(zeEventDestroy, (events[idx]));
    }
}

void ze_base_entry::close_lists() {
    if (comp_primitives.list)
        ZE_CALL(zeCommandListClose, (comp_primitives.list));
    if (copy_primitives.list)
        ZE_CALL(zeCommandListClose, (copy_primitives.list));
}

ze_kernel::ze_kernel(ze_module_handle_t module, const std::string &kernel_name, size_t worker_idx)
        : module(module),
          kernel_name(kernel_name),
          worker_idx(worker_idx) {
    global_data::get().ze_cache->get(worker_idx, module, kernel_name, &kernel);
    CCL_THROW_IF_NOT(kernel);
    LOG_DEBUG("get kernel: name: ", kernel_name);
}

ze_kernel::ze_kernel(ze_kernel &&other) noexcept
        : module(std::move(other.module)),
          kernel_name(std::move(other.kernel_name)),
          worker_idx(std::move(other.worker_idx)),
          group_count(std::move(other.group_count)),
          group_size(std::move(other.group_size)),
          kernel(std::move(other.kernel)) {
    other.module = nullptr;
    other.kernel_name.clear();
    other.worker_idx = 0;
    other.group_count = { 0, 0, 0 };
    other.group_size = { 0, 0, 0 };
    other.kernel = nullptr;
};

ze_kernel::~ze_kernel() {
    if (kernel) {
        global_data::get().ze_cache->push(worker_idx, module, kernel_name, kernel);
    }
}

void ze_kernel::set_args(ze_kernel_args_t kernel_args) {
    LOG_DEBUG("kernel ", kernel, " args:\n", to_string(kernel_args));
    set_kernel_args(kernel, kernel_args);
}

void ze_kernel::calculate_group_size(size_t count) {
    get_suggested_group_size(kernel, count, &group_size);
    LOG_DEBUG("suggested group size: ", to_string(group_size));

    ZE_CALL(zeKernelSetGroupSize,
            (kernel, group_size.groupSizeX, group_size.groupSizeY, group_size.groupSizeZ));

    get_suggested_group_count(group_size, count, &group_count);
    LOG_DEBUG("suggested group count: ", to_string(group_count));
}

ze_kernel_handle_t ze_kernel::get_kernel() const {
    return kernel;
}

const ze_group_count_t *ze_kernel::get_group_count() const {
    return &group_count;
}
