#include "ze_ipc_event_pool_manager.hpp"

#include "common/global/global.hpp"
#include "common/log/log.hpp"
#include "sched/entry/ze/ze_cache.hpp"

using namespace ccl;
using namespace ccl::ze;

void ipc_event_pool_manager::init(const ccl_stream* init_stream) {
    LOG_DEBUG("init");
    CCL_THROW_IF_NOT(init_stream, "no stream");

    context = init_stream->get_ze_context();
    CCL_THROW_IF_NOT(context, "context is not valid");
    LOG_DEBUG("init completed");
}

void ipc_event_pool_manager::clear() {
    for (const auto& pool_info : event_pool_info) {
        ccl::global_data::get().ze_cache->push(0, context, pool_info.second, pool_info.first);
    }
    event_pool_info.clear();
    LOG_DEBUG("finalize completed");
}

ze_event_pool_handle_t ipc_event_pool_manager::create(size_t event_count) {
    CCL_THROW_IF_NOT(context, "context is unavailable");

    ze_event_pool_desc_t event_pool_desc = default_event_pool_desc;
    event_pool_desc.count = event_count;
    event_pool_desc.flags = ZE_EVENT_POOL_FLAG_IPC | ZE_EVENT_POOL_FLAG_HOST_VISIBLE;

    ze_event_pool_handle_t event_pool{};
    ccl::global_data::get().ze_cache->get(0, context, event_pool_desc, &event_pool);
    CCL_THROW_IF_NOT(event_pool, "ipc event pool is unavailable");

    event_pool_info.push_back({ event_pool, event_pool_desc });

    LOG_DEBUG("created manager completed. event_pool_info.size: ", event_pool_info.size());
    return event_pool;
}
