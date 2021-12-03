#include "common/global/global.hpp"
#include "sched/entry/ze/ze_cache.hpp"
#include "sched/ze/ze_list_manager.hpp"

using namespace ccl;
using namespace ccl::ze;

list_manager::list_manager(ze_device_handle_t device, ze_context_handle_t context)
        : device(device),
          context(context) {
    LOG_DEBUG("create list manager");
    CCL_THROW_IF_NOT(device, "no device");
    CCL_THROW_IF_NOT(context, "no context");
    get_queues_properties(device, &queue_props);

    use_copy_queue =
        (global_data::env().ze_copy_engine != ccl_ze_copy_engine_none) && (queue_props.size() > 1);

    comp_queue = create_queue(init_mode::compute);

    if (use_copy_queue) {
        copy_queue = create_queue(init_mode::copy);
    }

    // Even if ze_copy_engine != ccl_ze_copy_engine_none,
    // copy queue can be created with ordinal and index equal comp queue ordinal and index,
    // it can cause deadlock for events between queues on card without blitter engine
    if (comp_queue.desc.ordinal == copy_queue.desc.ordinal &&
        comp_queue.desc.index == copy_queue.desc.index) {
        LOG_DEBUG(
            "copy queue has the ordinal equal to the ordinal of comp queue. Destroying copy queue...");
        free_queue(copy_queue);
    }
}

list_manager::list_manager(const ccl_stream* stream)
        : list_manager(stream->get_ze_device(), stream->get_ze_context()) {}

list_manager::~list_manager() {
    clear();
}

ze_command_list_handle_t list_manager::get_comp_list() {
    if (!comp_list) {
        comp_list = create_list(comp_queue);
        exec_list.push_back({ comp_queue, comp_list });
    }
    return comp_list.list;
}

ze_command_list_handle_t list_manager::get_copy_list() {
    if (use_copy_queue) {
        if (!copy_list) {
            copy_list = create_list(copy_queue);
            exec_list.push_back({ copy_queue, copy_list });
        }
        return copy_list.list;
    }
    return get_comp_list();
}

queue_info list_manager::create_queue(init_mode mode) {
    queue_info info{};
    uint32_t ordinal{}, queue_index{};
    if (mode == init_mode::copy) {
        LOG_DEBUG("create copy queue");
        get_copy_queue_ordinal(queue_props, &ordinal);
        info.is_copy = true;
    }
    else {
        LOG_DEBUG("create comp queue");
        get_comp_queue_ordinal(queue_props, &ordinal);
        info.is_copy = false;
    }
    get_queue_index(queue_props, ordinal, 0, &queue_index);

    info.desc = default_cmd_queue_desc;
    info.desc.index = queue_index;
    info.desc.ordinal = ordinal;

    global_data::get().ze_data->cache->get(worker_idx, context, device, info.desc, &info.queue);
    return info;
}

void list_manager::free_queue(queue_info& info) {
    if (!info)
        return;
    global_data::get().ze_data->cache->push(worker_idx, context, device, info.desc, info.queue);
    info.queue = nullptr;
    info.is_copy = false;
}

list_info list_manager::create_list(const queue_info& queue) {
    list_info info{};
    info.is_copy = queue.is_copy;
    info.desc = default_cmd_list_desc;
    info.desc.commandQueueGroupOrdinal = queue.desc.ordinal;
    LOG_DEBUG("create ", info.is_copy ? "copy" : "comp", " list");
    global_data::get().ze_data->cache->get(worker_idx, context, device, info.desc, &info.list);
    return info;
}

void list_manager::free_list(list_info& info) {
    if (!info)
        return;
    global_data::get().ze_data->cache->push(worker_idx, context, device, info.desc, info.list);
    info.list = nullptr;
    info.is_closed = false;
    info.is_copy = false;
}

void list_manager::clear() {
    LOG_DEBUG("destroy lists and queues");
    reset_execution_state();
    exec_list.clear();
    free_list(comp_list);
    free_list(copy_list);
    free_queue(comp_queue);
    free_queue(copy_queue);
}

void list_manager::reset_execution_state() {
    LOG_DEBUG("reset list manager execution state");
    executed = false;
}

bool list_manager::can_use_copy_queue() const {
    return use_copy_queue;
}

bool list_manager::is_executed() const {
    return executed;
}

void list_manager::execute_list(queue_info& queue, list_info& list) {
    if (list.list) {
        if (!list.is_closed) {
            ZE_CALL(zeCommandListClose, (list.list));
            list.is_closed = true;
        }
        ZE_CALL(zeCommandQueueExecuteCommandLists, (queue.queue, 1, &list.list, nullptr));
    }
}

void list_manager::execute() {
    CCL_THROW_IF_NOT(!executed, "lists are executed already");
    for (auto& list : exec_list) {
        LOG_DEBUG("execute ", list.first.is_copy ? "copy" : "comp", " list");
        execute_list(list.first, list.second);
    }
    executed = true;
}
