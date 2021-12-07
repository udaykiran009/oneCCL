#include "common/global/global.hpp"
#include "sched/entry/ze/ze_cache.hpp"
#include "sched/ze/ze_list_manager.hpp"

using namespace ccl;
using namespace ccl::ze;

ze_command_list_handle_t list_info::get_native() const {
    return list;
}

ze_command_list_handle_t* list_info::get_native_ptr() {
    return &list;
}

const ze_command_list_desc_t& list_info::get_desc() const {
    return desc;
}

bool list_info::is_valid() const {
    return list != nullptr;
}

bool list_info::is_copy() const {
    return is_copy_list;
}

uint32_t list_info::get_queue_index() const {
    return queue_index;
}

ze_command_queue_handle_t queue_info::get_native() const {
    return queue;
}

const ze_command_queue_desc_t& queue_info::get_desc() const {
    return desc;
}

bool queue_info::is_valid() const {
    return queue != nullptr;
}

bool queue_info::is_copy() const {
    return is_copy_queue;
}

queue_factory::queue_factory(ze_device_handle_t device, ze_context_handle_t context, bool is_copy)
        : device(device),
          context(context),
          is_copy_queue(is_copy) {
    ze_queue_properties_t queue_props;
    get_queues_properties(device, &queue_props);

    if (queue_props.size() == 1 && queue_props.front().numQueues == 1) {
        if (!global_data::env().disable_ze_family_check) {
            CCL_THROW_IF_NOT(get_device_family(device) == ccl::device_family::unknown,
                             "unknown device detected");
        }
    }

    uint32_t queue_count = 0;
    if (is_copy_queue) {
        get_copy_queue_ordinal(queue_props, &queue_ordinal);
        // if queue_ordinal == 0 here, then need to fix is_copy_queue selector
        // queue_ordinal == 0 should never happen here
        CCL_THROW_IF_NOT(queue_ordinal > 0, "selected comp ordinal for copy queue");
        queue_count = queue_props.at(queue_ordinal).numQueues;
    }
    else {
        get_comp_queue_ordinal(queue_props, &queue_ordinal);
        queue_count = queue_props.at(queue_ordinal).numQueues;
    }
    queues.resize(queue_count);
}

queue_factory::~queue_factory() {
    clear();
}

queue_info_t queue_factory::get(uint32_t index) {
    CCL_THROW_IF_NOT(!queues.empty(), "no queues");

    uint32_t queue_index = get_queue_index(index);
    auto& queue = queues[queue_index];
    if (!queue || !queue->is_valid()) {
        queue = std::make_shared<queue_info>();
        queue->desc = default_cmd_queue_desc;
        queue->desc.ordinal = queue_ordinal;
        queue->desc.index = queue_index;
        queue->is_copy_queue = is_copy_queue;
        global_data::get().ze_data->cache->get(
            worker_idx, context, device, queue->desc, &queue->queue);
        LOG_DEBUG("created new ",
                  queue_type(),
                  " queue: { ordinal: ",
                  queue_ordinal,
                  ", index: ",
                  queue_index,
                  " }");
    }
    return queue;
}

void queue_factory::destroy(queue_info_t& queue) {
    if (!queue || !queue->is_valid()) {
        return;
    }

    global_data::get().ze_data->cache->push(worker_idx, context, device, queue->desc, queue->queue);
    queue.reset();
}

const char* queue_factory::queue_type() const {
    return (is_copy_queue) ? "copy" : "comp";
}

uint32_t queue_factory::get_max_available_queue_count() const {
    uint32_t user_max_queues = queues.size();
    if (is_copy_queue) {
        user_max_queues = global_data::env().ze_max_copy_queues;
    }
    else {
        user_max_queues = global_data::env().ze_max_compute_queues;
    }
    return std::min(user_max_queues, static_cast<uint32_t>(queues.size()));
}

uint32_t queue_factory::get_queue_index(uint32_t requested_index) const {
    uint32_t max_queues = get_max_available_queue_count();
    uint32_t queue_index = requested_index % max_queues;
    queue_index = (queue_index + global_data::env().ze_queue_index_offset) % queues.size();
    return queue_index;
}

void queue_factory::clear() {
    for (auto& queue : queues) {
        destroy(queue);
    }
    queues.clear();
}

bool queue_factory::is_copy() const {
    return is_copy_queue;
}

bool queue_factory::can_use_copy_queue(ze_device_handle_t device) {
    ze_queue_properties_t queue_props;
    get_queues_properties(device, &queue_props);

    if (global_data::env().ze_copy_engine == ccl_ze_copy_engine_none) {
        return false;
    }

    uint32_t ordinal = 0;
    get_copy_queue_ordinal(queue_props, &ordinal);
    if (ordinal == 0) {
        return false;
    }

    return true;
}

list_factory::list_factory(ze_device_handle_t device, ze_context_handle_t context, bool is_copy)
        : device(device),
          context(context),
          is_copy_list(is_copy) {}

list_info_t list_factory::get(const queue_info_t& queue) {
    CCL_THROW_IF_NOT(queue && queue->is_valid(), "no queue");

    list_info_t list = std::make_shared<list_info>();
    list->desc = default_cmd_list_desc;
    list->desc.commandQueueGroupOrdinal = queue->get_desc().ordinal;
    list->is_copy_list = queue->is_copy();
    list->queue_index = queue->get_desc().index;
    global_data::get().ze_data->cache->get(worker_idx, context, device, list->desc, &list->list);
    LOG_DEBUG("created new ",
              list_type(),
              " list: { ordinal: ",
              list->desc.commandQueueGroupOrdinal,
              " } for queue: { ordinal: ",
              queue->get_desc().ordinal,
              ", index: ",
              list->queue_index,
              " }");
    return list;
}

void list_factory::destroy(list_info_t& list) {
    if (!list || !list->is_valid()) {
        return;
    }

    global_data::get().ze_data->cache->push(worker_idx, context, device, list->desc, list->list);
    list.reset();
}

const char* list_factory::list_type() const {
    return (is_copy_list) ? "copy" : "comp";
}

bool list_factory::is_copy() const {
    return is_copy_list;
}

list_manager::list_manager(const ccl_sched_base* sched, const ccl_stream* stream)
        : sched(sched),
          device(stream->get_ze_device()),
          context(stream->get_ze_context()) {
    LOG_DEBUG("create list manager");
    CCL_THROW_IF_NOT(device, "no device");
    CCL_THROW_IF_NOT(context, "no context");

    use_copy_queue = queue_factory::can_use_copy_queue(device);

    comp_queue_factory = std::make_unique<queue_factory>(device, context, false);
    comp_list_factory = std::make_unique<list_factory>(device, context, false);

    if (use_copy_queue) {
        copy_queue_factory = std::make_unique<queue_factory>(device, context, true);
        copy_list_factory = std::make_unique<list_factory>(device, context, true);
    }
}

list_manager::~list_manager() {
    clear();
}

list_info_t list_manager::get_list(const sched_entry* entry,
                                   uint32_t index,
                                   bool is_copy,
                                   const std::vector<ze_event_handle_t>& wait_events) {
    // get comp or copy primitives
    auto queue = (is_copy) ? copy_queue_factory->get(index) : comp_queue_factory->get(index);
    uint32_t queue_index = queue->get_desc().index;
    auto& map = (is_copy) ? copy_queue_map : comp_queue_map;

    // get queue from map. if no list for this queue, then create new one
    bool new_list_for_queue = false;
    auto& list_pair = map[queue_index];
    auto& list = list_pair.first;
    if ((!list || !list->is_valid()) && sched->use_single_list) {
        new_list_for_queue = true;
    }
    else if (!sched->use_single_list) {
        new_list_for_queue = true;
    }

    // check if current entry is used list at the first time
    // it is needed to append wait events from this entry to the list if single_List mode is active
    bool new_entry_for_list = false;
    auto& list_entry_map = list_pair.second;
    auto found = list_entry_map.find(entry);
    if (found == list_entry_map.end()) {
        // remember entry. value in the map is not used
        list_entry_map.insert({ entry, true });
        new_entry_for_list = true;
    }

    // if we dont have any lists for current queue
    if (new_list_for_queue && new_entry_for_list) {
        // creaete new list
        list = (is_copy) ? copy_list_factory->get(queue) : comp_list_factory->get(queue);
        access_list.push_back({ queue, list });
        // remember list for current entry
        entry_map[entry].push_back(std::make_pair(queue, list));
        LOG_DEBUG("[entry ",
                  entry->name(),
                  "] created new ",
                  list->is_copy() ? "copy" : "comp",
                  " list with queue index ",
                  list->get_queue_index(),
                  ". total list count ",
                  access_list.size());
    }

    // if single_list mode is active and current entry never use this list before,
    // then append wait events from that entry,
    // because we must wait commands from the previous entries (which can be in other lists too)
    if (new_entry_for_list && sched->use_single_list && !wait_events.empty()) {
        LOG_DEBUG("[entry ",
                  entry->name(),
                  "] append wait events to ",
                  list->is_copy() ? "copy" : "comp",
                  " list with queue index ",
                  list->get_queue_index());
        ZE_CALL(zeCommandListAppendWaitOnEvents,
                (list->get_native(),
                 wait_events.size(),
                 const_cast<ze_event_handle_t*>(wait_events.data())));
    }

    return list;
}

ze_command_list_handle_t list_manager::get_comp_list(
    const sched_entry* entry,
    const std::vector<ze_event_handle_t>& wait_events,
    uint32_t index) {
    auto list = get_list(entry, index, false, wait_events);
    return list->get_native();
}

ze_command_list_handle_t list_manager::get_copy_list(
    const sched_entry* entry,
    const std::vector<ze_event_handle_t>& wait_events,
    uint32_t index) {
    if (use_copy_queue) {
        auto list = get_list(entry, index, true, wait_events);
        return list->get_native();
    }
    else {
        return get_comp_list(entry, wait_events, index);
    }
}

void list_manager::clear() {
    LOG_DEBUG("destroy lists and queues");
    reset_execution_state();
    comp_queue_map.clear();
    copy_queue_map.clear();
    for (auto& queue_list_pair : access_list) {
        auto& list = queue_list_pair.second;
        if (list->is_copy()) {
            copy_list_factory->destroy(list);
        }
        else {
            comp_list_factory->destroy(list);
        }
    }
    access_list.clear();
    entry_map.clear();

    if (comp_queue_factory) {
        comp_queue_factory->clear();
    }
    if (copy_queue_factory) {
        copy_queue_factory->clear();
    }
}

void list_manager::reset_execution_state() {
    LOG_DEBUG("reset list manager execution state");
    executed = false;
    for (auto& queue_list_pair : access_list) {
        auto& list = queue_list_pair.second;
        CCL_THROW_IF_NOT(list->is_closed, "detected list that has not been closed");
        list->is_executed = false;
    }
}

bool list_manager::can_use_copy_queue() const {
    return use_copy_queue;
}

bool list_manager::is_executed() const {
    return executed;
}

void list_manager::execute_list(queue_info_t& queue, list_info_t& list) {
    CCL_THROW_IF_NOT(list && list->is_valid(), "trying to execute uninitialized list");
    CCL_THROW_IF_NOT(queue && queue->is_valid(), "trying to execute list on uninitialized queue");
    CCL_THROW_IF_NOT(!list->is_executed, "trying to execute list that already has been executed");
    CCL_THROW_IF_NOT((queue->is_copy() && list->is_copy()) || !queue->is_copy(),
                     "trying to execute comp list on copy queue");

    if (!list->is_closed) {
        ZE_CALL(zeCommandListClose, (list->get_native()));
        list->is_closed = true;
    }
    LOG_DEBUG("execute ",
              list->is_copy() ? "copy" : "comp",
              " list with queue index ",
              queue->get_desc().index);

    ZE_CALL(zeCommandQueueExecuteCommandLists,
            (queue->get_native(), 1, list->get_native_ptr(), nullptr));
    list->is_executed = true;
}

void list_manager::execute(const sched_entry* entry) {
    CCL_THROW_IF_NOT(!sched->use_single_list || !executed, "lists are executed already");
    LOG_DEBUG("execute ", entry->name(), " entry");

    if (global_data::env().enable_ze_list_dump) {
        print_dump();
    }

    auto& container = (sched->use_single_list) ? access_list : entry_map[entry];
    for (auto& queue_list_pair : container) {
        auto& queue = queue_list_pair.first;
        auto& list = queue_list_pair.second;
        execute_list(queue, list);
    }

    executed = true;
}

void list_manager::print_dump() const {
    std::unordered_map<uint32_t, size_t> comp_queue_index_count;
    std::unordered_map<int, size_t> copy_queue_index_count;
    size_t comp_list_count = 0;
    size_t copy_list_count = 0;

    std::stringstream ss;
    ss << std::endl;
    for (auto& queue_list_pair : access_list) {
        auto& queue = queue_list_pair.first;
        uint32_t queue_index = queue->get_desc().index;
        if (queue->is_copy()) {
            copy_list_count++;
            if (copy_queue_index_count.count(queue_index)) {
                copy_queue_index_count[queue_index]++;
            }
            else {
                copy_queue_index_count.insert({ queue_index, 1 });
            }
        }
        else {
            comp_list_count++;
            if (comp_queue_index_count.count(queue_index)) {
                comp_queue_index_count[queue_index]++;
            }
            else {
                comp_queue_index_count.insert({ queue_index, 1 });
            }
        }
    }

    ss << "comp list count: " << comp_list_count << std::endl;
    if (!comp_queue_index_count.empty()) {
        ss << "comp queues usage (unsorted): {" << std::endl;
        for (auto& index : comp_queue_index_count) {
            ss << "  index: " << index.first << " list count: " << index.second << std::endl;
        }
        ss << "}" << std::endl;
    }

    ss << "copy list count: " << copy_list_count << std::endl;
    if (!copy_queue_index_count.empty()) {
        ss << "copy queues usage (unsorted): {" << std::endl;
        for (auto& index : copy_queue_index_count) {
            ss << "  index: " << index.first << " list count: " << index.second << std::endl;
        }
        ss << "}" << std::endl;
    }
    ss << "total list count: " << access_list.size() << std::endl;

    if (sched->use_single_list && !access_list.empty()) {
        ss << "execution order: {" << std::endl;
        for (auto& queue_list_pair : access_list) {
            auto& list = queue_list_pair.second;
            ss << "  index: " << list->get_queue_index()
               << ", type: " << ((list->is_copy()) ? "copy" : "comp") << std::endl;
        }
        ss << "}" << std::endl;
    }

    logger.info(ss.str());
}
