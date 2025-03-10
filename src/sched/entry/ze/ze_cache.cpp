#include "common/global/global.hpp"
#include "sched/entry/ze/ze_cache.hpp"

#include <fcntl.h>
#include <iterator>

namespace ccl {
namespace ze {

template <class map_t, class... keys_t>
bool get_from_cache(map_t& cache, typename map_t::mapped_type& object, keys_t... keys) {
    bool success{};

    if (!global_data::env().enable_ze_cache)
        return success;

    typename map_t::key_type key(keys...);
    auto key_value = cache.find(key);
    if (key_value != cache.end()) {
        object = key_value->second;
        cache.erase(key_value);
        LOG_DEBUG("loaded from cache: object: ", object);
        success = true;
    }
    return success;
}

template <class map_t, class... keys_t>
bool push_to_cache(map_t& cache, const typename map_t::mapped_type& object, keys_t... keys) {
    bool success{};

    if (!global_data::env().enable_ze_cache)
        return success;

    typename map_t::key_type key(keys...);
    auto range = cache.equal_range(key);
    auto range_len = std::distance(range.first, range.second);
    if (range_len > 0) {
        LOG_DEBUG("cache already contain ", range_len, " objects with the same key");
        for (auto i = range.first; i != range.second; ++i) {
            CCL_THROW_IF_NOT(i->second != object, "trying to push object that already exists");
        }
    }
    cache.insert({ std::move(key), object });
    LOG_DEBUG("inserted to cache: object: ", object);
    success = true;
    return success;
}

// kernel_cache
kernel_cache::~kernel_cache() {
    if (!cache.empty()) {
        LOG_WARN("kernel cache is not empty, size: ", cache.size());
        clear();
    }
}

void kernel_cache::clear() {
    LOG_DEBUG("clear kernel cache: size: ", cache.size());
    std::lock_guard<std::mutex> lock(mutex);
    for (auto& key_value : cache) {
        ZE_CALL(zeKernelDestroy, (key_value.second));
    }
    cache.clear();
}

void kernel_cache::get(ze_module_handle_t module,
                       const std::string& kernel_name,
                       ze_kernel_handle_t* kernel) {
    CCL_THROW_IF_NOT(module);
    CCL_THROW_IF_NOT(!kernel_name.empty());
    CCL_THROW_IF_NOT(kernel);
    std::lock_guard<std::mutex> lock(mutex);
    if (!get_from_cache(cache, *kernel, module, kernel_name)) {
        create_kernel(module, kernel_name, kernel);
    }
}

void kernel_cache::push(ze_module_handle_t module,
                        const std::string& kernel_name,
                        ze_kernel_handle_t kernel) {
    CCL_THROW_IF_NOT(module);
    CCL_THROW_IF_NOT(!kernel_name.empty());
    CCL_THROW_IF_NOT(kernel);
    std::lock_guard<std::mutex> lock(mutex);
    if (!push_to_cache(cache, kernel, module, kernel_name)) {
        ZE_CALL(zeKernelDestroy, (kernel));
    }
}

// list_cache
list_cache::~list_cache() {
    if (!cache.empty()) {
        LOG_WARN("list cache is not empty, size: ", cache.size());
        clear();
    }
}

void list_cache::clear() {
    LOG_DEBUG("clear list cache: size: ", cache.size());
    std::lock_guard<std::mutex> lock(mutex);
    for (auto& key_value : cache) {
        ZE_CALL(zeCommandListDestroy, (key_value.second));
    }
    cache.clear();
}

void list_cache::get(ze_context_handle_t context,
                     ze_device_handle_t device,
                     const ze_command_list_desc_t& list_desc,
                     ze_command_list_handle_t* list) {
    CCL_THROW_IF_NOT(context);
    CCL_THROW_IF_NOT(device);
    CCL_THROW_IF_NOT(list);
    std::lock_guard<std::mutex> lock(mutex);
    if (get_from_cache(
            cache, *list, context, device, list_desc.commandQueueGroupOrdinal, list_desc.flags)) {
        ZE_CALL(zeCommandListReset, (*list));
    }
    else {
        ZE_CALL(zeCommandListCreate, (context, device, &list_desc, list));
    }
}

void list_cache::push(ze_context_handle_t context,
                      ze_device_handle_t device,
                      const ze_command_list_desc_t& list_desc,
                      ze_command_list_handle_t list) {
    CCL_THROW_IF_NOT(context);
    CCL_THROW_IF_NOT(device);
    CCL_THROW_IF_NOT(list);
    std::lock_guard<std::mutex> lock(mutex);
    if (!push_to_cache(
            cache, list, context, device, list_desc.commandQueueGroupOrdinal, list_desc.flags)) {
        ZE_CALL(zeCommandListDestroy, (list));
    }
}

// queue_cache
queue_cache::~queue_cache() {
    if (!cache.empty()) {
        LOG_WARN("queue cache is not empty, size: ", cache.size());
        clear();
    }
}

void queue_cache::clear() {
    LOG_DEBUG("clear queue cache: size: ", cache.size());
    std::lock_guard<std::mutex> lock(mutex);
    for (auto& key_value : cache) {
        ZE_CALL(zeCommandQueueDestroy, (key_value.second));
    }
    cache.clear();
}

void queue_cache::get(ze_context_handle_t context,
                      ze_device_handle_t device,
                      const ze_command_queue_desc_t& queue_desc,
                      ze_command_queue_handle_t* queue) {
    CCL_THROW_IF_NOT(context);
    CCL_THROW_IF_NOT(device);
    CCL_THROW_IF_NOT(queue);
    std::lock_guard<std::mutex> lock(mutex);
    if (!get_from_cache(cache,
                        *queue,
                        context,
                        device,
                        queue_desc.index,
                        queue_desc.ordinal,
                        queue_desc.flags,
                        queue_desc.mode,
                        queue_desc.priority)) {
        ZE_CALL(zeCommandQueueCreate, (context, device, &queue_desc, queue));
    }
}

void queue_cache::push(ze_context_handle_t context,
                       ze_device_handle_t device,
                       const ze_command_queue_desc_t& queue_desc,
                       ze_command_queue_handle_t queue) {
    CCL_THROW_IF_NOT(context);
    CCL_THROW_IF_NOT(device);
    CCL_THROW_IF_NOT(queue);
    std::lock_guard<std::mutex> lock(mutex);
    if (!push_to_cache(cache,
                       queue,
                       context,
                       device,
                       queue_desc.index,
                       queue_desc.ordinal,
                       queue_desc.flags,
                       queue_desc.mode,
                       queue_desc.priority)) {
        ZE_CALL(zeCommandQueueDestroy, (queue));
    }
}

// event_pool_cache
event_pool_cache::~event_pool_cache() {
    if (!cache.empty()) {
        LOG_WARN("event pool cache is not empty, size: ", cache.size());
        clear();
    }
}

void event_pool_cache::clear() {
    LOG_DEBUG("clear event pool cache: size: ", cache.size());
    std::lock_guard<std::mutex> lock(mutex);
    for (auto& key_value : cache) {
        ZE_CALL(zeEventPoolDestroy, (key_value.second));
    }
    cache.clear();
}

void event_pool_cache::get(ze_context_handle_t context,
                           const ze_event_pool_desc_t& pool_desc,
                           ze_event_pool_handle_t* event_pool) {
    CCL_THROW_IF_NOT(context);
    CCL_THROW_IF_NOT(event_pool);
    std::lock_guard<std::mutex> lock(mutex);
    // TODO: we can potentially use pool with count >= pool_desc.count
    if (!get_from_cache(cache, *event_pool, context, pool_desc.flags, pool_desc.count)) {
        ZE_CALL(zeEventPoolCreate, (context, &pool_desc, 0, nullptr, event_pool));
    }
}

void event_pool_cache::push(ze_context_handle_t context,
                            const ze_event_pool_desc_t& pool_desc,
                            ze_event_pool_handle_t event_pool) {
    CCL_THROW_IF_NOT(context);
    CCL_THROW_IF_NOT(event_pool);
    std::lock_guard<std::mutex> lock(mutex);
    if (!push_to_cache(cache, event_pool, context, pool_desc.flags, pool_desc.count)) {
        ZE_CALL(zeEventPoolDestroy, (event_pool));
    }
}

// device_mem_cache
device_mem_cache::~device_mem_cache() {
    if (!cache.empty()) {
        LOG_WARN("device memory cache is not empty, size: ", cache.size());
        clear();
    }
}

void device_mem_cache::clear() {
    LOG_DEBUG("clear device memory cache: size: ", cache.size());
    std::lock_guard<std::mutex> lock(mutex);
    //for (auto& key_value : cache) {
    // TODO: there is a segfault on this call, when ~cache is invoked w/ or w/0 api cache.
    // But it passes, when CCL_ZE_CACHE=0 (calls of zeMemAllocDevice and ZeMemFree happen on every iteration).
    // We don't control destroying phase and may be key_value.second (mem_ptr) is already away to free?
    // ZE_CALL(zeMemFree, (std::get<0>(key_value.first), key_value.second))
    //}
    cache.clear();
}

void device_mem_cache::get(ze_context_handle_t context,
                           ze_device_handle_t device,
                           const ze_device_mem_alloc_desc_t& device_mem_alloc_desc,
                           size_t bytes,
                           size_t alignment,
                           void** pptr) {
    CCL_THROW_IF_NOT(context);
    CCL_THROW_IF_NOT(device);
    CCL_THROW_IF_NOT(pptr);
    std::lock_guard<std::mutex> lock(mutex);
    if (!get_from_cache(cache,
                        *pptr,
                        context,
                        device,
                        bytes,
                        device_mem_alloc_desc.flags,
                        device_mem_alloc_desc.ordinal)) {
        ZE_CALL(zeMemAllocDevice,
                (context, &device_mem_alloc_desc, bytes, alignment, device, pptr));
    }
}

void device_mem_cache::push(ze_context_handle_t context,
                            ze_device_handle_t device,
                            const ze_device_mem_alloc_desc_t& device_mem_alloc_desc,
                            size_t bytes,
                            size_t alignment,
                            void* ptr) {
    CCL_THROW_IF_NOT(context);
    CCL_THROW_IF_NOT(device);
    CCL_THROW_IF_NOT(ptr);
    std::lock_guard<std::mutex> lock(mutex);
    if (!push_to_cache(cache,
                       ptr,
                       context,
                       device,
                       bytes,
                       device_mem_alloc_desc.flags,
                       device_mem_alloc_desc.ordinal)) {
        ZE_CALL(zeMemFree, (context, ptr));
    }
}

// module_cache
module_cache::~module_cache() {
    if (!cache.empty()) {
        LOG_WARN("module cache is not empty, size: ", cache.size());
        clear();
    }
}

void module_cache::clear() {
    LOG_DEBUG("clear module cache: size: ", cache.size());
    std::lock_guard<std::mutex> lock(mutex);
    for (auto& key_value : cache) {
        ZE_CALL(zeModuleDestroy, (key_value.second));
    }
    cache.clear();
}

void module_cache::get(ze_context_handle_t context,
                       ze_device_handle_t device,
                       const std::string& spv_name,
                       ze_module_handle_t* module) {
    CCL_THROW_IF_NOT(context);
    CCL_THROW_IF_NOT(device);
    CCL_THROW_IF_NOT(!spv_name.empty());
    CCL_THROW_IF_NOT(module);
    std::lock_guard<std::mutex> lock(mutex);
    key_t key(device, spv_name);
    auto key_value = cache.find(key);
    if (key_value != cache.end()) {
        *module = key_value->second;
        LOG_DEBUG("loaded from cache: module: ", *module);
    }
    else {
        load(context, device, spv_name, module);
        cache.insert({ std::move(key), *module });
        LOG_DEBUG("inserted to cache: module: ", *module);
    }
}

void module_cache::load(ze_context_handle_t context,
                        ze_device_handle_t device,
                        const std::string& spv_name,
                        ze_module_handle_t* module) {
    CCL_THROW_IF_NOT(context);
    CCL_THROW_IF_NOT(device);
    CCL_THROW_IF_NOT(!spv_name.empty());
    CCL_THROW_IF_NOT(module);
    const std::string& modules_dir = global_data::env().kernel_path;
    std::string file_path = modules_dir + spv_name;
    load_module(file_path, device, context, module);
}

mem_handle_cache::mem_handle_cache() {
    if (!global_data::env().enable_ze_cache_ipc_handles) {
        return;
    }

    threshold = global_data::env().ze_cache_ipc_handles_threshold;
    cache.reserve(threshold + 1);
    LOG_DEBUG("cache threshold: ", threshold);
}

mem_handle_cache::~mem_handle_cache() {
    if (!cache.empty()) {
        LOG_WARN("mem handle cache is not empty, size: ", cache.size());
        clear();
    }
}

void mem_handle_cache::clear() {
    std::lock_guard<std::mutex> lock(mutex);
    LOG_DEBUG("clear cache size: ", cache.size());
    make_clean(0);
    cache.clear();
    cache_list.clear();
}

void mem_handle_cache::make_clean(size_t limit) {
    while (cache.size() > limit) {
        auto it = cache_list.end();
        --it;
        // handle will be closed from handle_desc class destructor
        cache.erase(it->first);
        cache_list.pop_back();
    }
}

void mem_handle_cache::get(ze_context_handle_t context,
                           ze_device_handle_t device,
                           const ipc_handle_desc& info,
                           value_t* out_value) {
    CCL_THROW_IF_NOT(context);
    CCL_THROW_IF_NOT(device);
    CCL_THROW_IF_NOT(info.remote_context_id >= 0);
    CCL_THROW_IF_NOT(info.remote_device_id >= 0);
    std::lock_guard<std::mutex> lock(mutex);

    bool found{};
    key_t key(info.remote_pid,
              info.remote_mem_alloc_id,
              info.remote_context_id,
              info.remote_device_id,
              context,
              device);
    auto key_value = cache.find(key);
    if (key_value != cache.end()) {
        value_t& value = key_value->second->second;
        const auto& handle_from_cache = value->handle;
        int fd = get_fd_from_handle(handle_from_cache);
        if (fd_is_valid(fd)) {
            // move key_value to the beginning of the list
            cache_list.splice(cache_list.begin(), cache_list, key_value->second);
            close_handle_fd(info.ipc_handle);
            *out_value = value;
            found = true;
        }
        else {
            LOG_DEBUG("handle is invalid in the cache");
            cache_list.erase(key_value->second);
            cache.erase(key_value);
            found = false;
        }
    }

    if (!found) {
        push(device, std::move(key), info.ipc_handle, out_value);
    }
}

void mem_handle_cache::push(ze_device_handle_t device,
                            key_t&& key,
                            const ze_ipc_mem_handle_t& handle,
                            value_t* out_value) {
    make_clean(threshold);

    // no mutex here because this method is private and called from get() only
    auto remote_context_id = std::get<static_cast<size_t>(key_id::remote_context_id)>(key);
    auto remote_context = global_data::get().ze_data->contexts.at(remote_context_id);

    void* ptr{};
    ZE_CALL(zeMemOpenIpcHandle, (remote_context, device, handle, {}, &ptr));
    *out_value = std::make_shared<const handle_desc>(remote_context, handle, ptr);
    cache_list.push_front(std::make_pair(key, *out_value));
    cache.insert(std::make_pair(key, cache_list.begin()));
}

bool mem_handle_cache::fd_is_valid(int fd) {
    return fcntl(fd, F_GETFD) != -1 || errno != EBADF;
}

mem_handle_cache::handle_desc::handle_desc(ze_context_handle_t remote_context,
                                           const ze_ipc_mem_handle_t& handle,
                                           const void* ptr)
        : remote_context(remote_context),
          handle(handle),
          ptr(ptr) {}

const void* mem_handle_cache::handle_desc::get_ptr() const {
    return ptr;
}

void mem_handle_cache::handle_desc::close_handle() const {
    CCL_THROW_IF_NOT(remote_context, " no remote context");

    if (global_data::env().ze_close_ipc_wa) {
        close_handle_fd(handle);
        return;
    }

    auto fd = get_fd_from_handle(handle);
    auto res = zeMemCloseIpcHandle(remote_context, ptr);
    if (res == ZE_RESULT_ERROR_INVALID_ARGUMENT) {
        close(fd);
    }
    else if (res != ZE_RESULT_SUCCESS) {
        CCL_THROW("error at zeMemCloseIpcHandle, code: ", to_string(res));
    }
}

mem_handle_cache::handle_desc::~handle_desc() {
    close_handle();
}

// cache
cache::~cache() {
    for (size_t i = 0; i < instance_count; ++i) {
        kernels[i].clear();
        lists[i].clear();
        queues[i].clear();
        event_pools[i].clear();
        device_mems[i].clear();
    }

    modules.clear();
    mem_handles.clear();
}

} // namespace ze
} // namespace ccl
