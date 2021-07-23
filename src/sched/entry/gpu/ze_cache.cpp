#include "common/global/global.hpp"
#include "ze_cache.hpp"

namespace ccl {
namespace ze {

fence_cache::~fence_cache() {
    if (!cache.empty()) {
        LOG_WARN("fence cache is not empty, size: ", cache.size());
    }
    clear();
}

void fence_cache::clear() {
    LOG_DEBUG("clear fence cache: size: ", cache.size());
    for (auto& key_value : cache) {
        ZE_CALL(zeFenceDestroy(key_value.second));
    }
    cache.clear();
}

void fence_cache::get(ze_command_queue_handle_t queue,
                      ze_fence_desc_t* fence_desc,
                      ze_fence_handle_t* fence) {
    if (ccl::global_data::env().enable_kernel_cache) {
        key_t key(queue, fence_desc->flags);
        auto key_value = cache.find(key);
        if (key_value != cache.end()) {
            *fence = key_value->second;
            ZE_CALL(zeFenceReset(*fence));
            cache.erase(key_value);
            LOG_DEBUG("loaded from cache: fence: ", *fence);
            return;
        }
    }
    ZE_CALL(zeFenceCreate(queue, fence_desc, fence));
}

void fence_cache::push(ze_command_queue_handle_t queue,
                       ze_fence_desc_t* fence_desc,
                       ze_fence_handle_t* fence) {
    if (ccl::global_data::env().enable_kernel_cache) {
        key_t key(queue, fence_desc->flags);
        cache.insert({ std::move(key), *fence });
        LOG_DEBUG("inserted to cache: fence: ", *fence);
        return;
    }
    zeFenceDestroy(*fence);
}

kernel_cache::~kernel_cache() {
    if (!cache.empty()) {
        LOG_WARN("kernel cache is not empty, size: ", cache.size());
    }
    clear();
}

void kernel_cache::clear() {
    LOG_DEBUG("clear kernel cache: size: ", cache.size());
    for (auto& key_value : cache) {
        ZE_CALL(zeKernelDestroy(key_value.second));
    }
    cache.clear();
}

void kernel_cache::get(ze_module_handle_t module,
                       const std::string& kernel_name,
                       ze_kernel_handle_t* kernel) {
    if (ccl::global_data::env().enable_kernel_cache) {
        key_t key(module, kernel_name);
        auto key_value = cache.find(key);
        if (key_value != cache.end()) {
            *kernel = key_value->second;
            cache.erase(key_value);
            LOG_DEBUG("loaded from cache: { name: ", kernel_name, ", kernel: ", *kernel, " }");
            return;
        }
    }
    create_kernel(module, kernel_name, kernel);
}

void kernel_cache::push(ze_module_handle_t module,
                        const std::string& kernel_name,
                        ze_kernel_handle_t* kernel) {
    if (ccl::global_data::env().enable_kernel_cache) {
        key_t key(module, kernel_name);
        cache.insert({ std::move(key), *kernel });
        LOG_DEBUG("inserted to cache: { name: ", kernel_name, ", kernel: ", *kernel, " }");
        return;
    }
    ZE_CALL(zeKernelDestroy(*kernel));
}

list_cache::~list_cache() {
    if (!cache.empty()) {
        LOG_WARN("list cache is not empty, size: ", cache.size());
    }
    clear();
}

void list_cache::clear() {
    LOG_DEBUG("clear list cache: size: ", cache.size());
    for (auto& key_value : cache) {
        ZE_CALL(zeCommandListDestroy(key_value.second));
    }
    cache.clear();
}

void list_cache::get(ze_context_handle_t context,
                     ze_device_handle_t device,
                     ze_command_list_desc_t* list_desc,
                     ze_command_list_handle_t* list) {
    if (ccl::global_data::env().enable_kernel_cache) {
        key_t key(context, device, list_desc->commandQueueGroupOrdinal, list_desc->flags);
        auto key_value = cache.find(key);
        if (key_value != cache.end()) {
            *list = key_value->second;
            ZE_CALL(zeCommandListReset(*list));
            cache.erase(key_value);
            LOG_DEBUG("loaded from cache: list: ", *list);
            return;
        }
    }
    ZE_CALL(zeCommandListCreate(context, device, list_desc, list));
}

void list_cache::push(ze_context_handle_t context,
                      ze_device_handle_t device,
                      ze_command_list_desc_t* list_desc,
                      ze_command_list_handle_t* list) {
    if (ccl::global_data::env().enable_kernel_cache) {
        key_t key(context, device, list_desc->commandQueueGroupOrdinal, list_desc->flags);
        cache.insert({ std::move(key), *list });
        LOG_DEBUG("inserted to cache: list: ", *list);
        return;
    }
    ZE_CALL(zeCommandListDestroy(*list));
}

queue_cache::~queue_cache() {
    if (!cache.empty()) {
        LOG_WARN("queue cache is not empty, size: ", cache.size());
    }
    clear();
}

void queue_cache::clear() {
    LOG_DEBUG("clear queue cache: size: ", cache.size());
    for (auto& key_value : cache) {
        ZE_CALL(zeCommandQueueDestroy(key_value.second))
    }
    cache.clear();
}

void queue_cache::get(ze_context_handle_t context,
                      ze_device_handle_t device,
                      ze_command_queue_desc_t* queue_desc,
                      ze_command_queue_handle_t* queue) {
    if (ccl::global_data::env().enable_kernel_cache) {
        key_t key(context,
                  device,
                  queue_desc->index,
                  queue_desc->ordinal,
                  queue_desc->flags,
                  queue_desc->mode,
                  queue_desc->priority);
        auto key_value = cache.find(key);
        if (key_value != cache.end()) {
            *queue = key_value->second;
            cache.erase(key_value);
            LOG_DEBUG("loaded from cache: queue: ", *queue);
            return;
        }
    }
    ZE_CALL(zeCommandQueueCreate(context, device, queue_desc, queue));
}

void queue_cache::push(ze_context_handle_t context,
                       ze_device_handle_t device,
                       ze_command_queue_desc_t* queue_desc,
                       ze_command_queue_handle_t* queue) {
    if (ccl::global_data::env().enable_kernel_cache) {
        key_t key(context,
                  device,
                  queue_desc->index,
                  queue_desc->ordinal,
                  queue_desc->flags,
                  queue_desc->mode,
                  queue_desc->priority);
        cache.insert({ std::move(key), *queue });
        LOG_DEBUG("inserted to cache: queue: ", *queue);
        return;
    }
    ZE_CALL(zeCommandQueueDestroy(*queue));
}

module_cache::~module_cache() {
    clear();
}

void module_cache::clear() {
    LOG_DEBUG("clear module cache: size: ", cache.size());
    std::lock_guard<std::mutex> lock(mutex);
    for (auto& key_value : cache) {
        ZE_CALL(zeModuleDestroy(key_value.second))
    }
    cache.clear();
}

void module_cache::get(ze_context_handle_t context,
                       ze_device_handle_t device,
                       ze_module_handle_t* module) {
    std::lock_guard<std::mutex> lock(mutex);
    auto key_value = cache.find(device);
    if (key_value != cache.end()) {
        *module = key_value->second;
        LOG_DEBUG("loaded from cache: module: ", *module);
    }
    else {
        load(context, device, module);
        cache.insert({ device, *module });
        LOG_DEBUG("inserted to cache: module: ", *module);
    }
}

void module_cache::load(ze_context_handle_t context,
                        ze_device_handle_t device,
                        ze_module_handle_t* module) {
    std::string modules_dir = ccl::global_data::env().kernel_path;
    // TODO: remove
    if (modules_dir.empty()) {
        std::string ccl_root = getenv("CCL_ROOT");
        CCL_THROW_IF_NOT(!ccl_root.empty(), "incorrect comm kernels path, CCL_ROOT not found!");
        modules_dir = ccl_root + "/lib/kernels/";
    }
    load_module(modules_dir, "ring_allreduce.spv", device, context, module);
}

cache::~cache() {
    for (auto& instance : fences) {
        instance.clear();
    }

    for (auto& instance : kernels) {
        instance.clear();
    }

    for (auto& instance : lists) {
        instance.clear();
    }

    for (auto& instance : queues) {
        instance.clear();
    }

    modules.clear();
}

} // namespace ze
} // namespace ccl
