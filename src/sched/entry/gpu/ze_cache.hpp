#pragma once

#include "common/utils/hash.hpp"
#include "ze_primitives.hpp"

#include <unordered_map>

namespace ccl {
namespace ze {

class fence_cache {
public:
    fence_cache() = default;
    ~fence_cache();

    void clear();

    void get(ze_command_queue_handle_t queue,
             ze_fence_desc_t* fence_desc,
             ze_fence_handle_t* fence);
    void push(ze_command_queue_handle_t queue,
              ze_fence_desc_t* fence_desc,
              ze_fence_handle_t* fence);

private:
    using key_t = typename std::tuple<ze_command_queue_handle_t, ze_fence_flags_t>;
    using value_t = ze_fence_handle_t;
    std::unordered_multimap<key_t, value_t, utils::tuple_hash> cache;
};

class kernel_cache {
public:
    kernel_cache() = default;
    ~kernel_cache();

    void clear();

    void get(ze_module_handle_t module, const std::string& kernel_name, ze_kernel_handle_t* kernel);
    void push(ze_module_handle_t module,
              const std::string& kernel_name,
              ze_kernel_handle_t* kernel);

private:
    using key_t = typename std::tuple<ze_module_handle_t, std::string>;
    using value_t = ze_kernel_handle_t;
    std::unordered_multimap<key_t, value_t, utils::tuple_hash> cache;
};

// TODO: need to improve with ability to save list with commands for specific algo
class list_cache {
public:
    list_cache() = default;
    ~list_cache();

    void clear();

    void get(ze_context_handle_t context,
             ze_device_handle_t device,
             ze_command_list_desc_t* list_desc,
             ze_command_list_handle_t* list);
    void push(ze_context_handle_t context,
              ze_device_handle_t device,
              ze_command_list_desc_t* list_desc,
              ze_command_list_handle_t* list);

private:
    using key_t = typename std::
        tuple<ze_context_handle_t, ze_device_handle_t, uint32_t, ze_command_list_flags_t>;
    using value_t = ze_command_list_handle_t;
    std::unordered_multimap<key_t, value_t, utils::tuple_hash> cache;
};

class queue_cache {
public:
    queue_cache() = default;
    ~queue_cache();

    void clear();

    void get(ze_context_handle_t context,
             ze_device_handle_t device,
             ze_command_queue_desc_t* queue_desc,
             ze_command_queue_handle_t* queue);
    void push(ze_context_handle_t context,
              ze_device_handle_t device,
              ze_command_queue_desc_t* queue_desc,
              ze_command_queue_handle_t* queue);

private:
    using key_t = typename std::tuple<ze_context_handle_t,
                                      ze_device_handle_t,
                                      uint32_t,
                                      uint32_t,
                                      ze_command_queue_flags_t,
                                      ze_command_queue_mode_t,
                                      ze_command_queue_priority_t>;
    using value_t = ze_command_queue_handle_t;
    std::unordered_multimap<key_t, value_t, utils::tuple_hash> cache;
};

class module_cache {
public:
    module_cache() = default;
    ~module_cache();

    void clear();

    void get(ze_context_handle_t context, ze_device_handle_t device, ze_module_handle_t* module);

private:
    using key_t = ze_device_handle_t;
    using value_t = ze_module_handle_t;
    std::unordered_multimap<ze_device_handle_t, ze_module_handle_t> cache;
    std::mutex mutex;

    void load(ze_context_handle_t context, ze_device_handle_t device, ze_module_handle_t* module);
};

class cache {
public:
    cache(size_t instance_count)
            : fences(instance_count),
              kernels(instance_count),
              lists(instance_count),
              queues(instance_count) {}
    cache(const cache&) = delete;
    cache& operator=(const cache&) = delete;
    ~cache();

    /* get */
    void get(size_t worker_idx,
             ze_command_queue_handle_t queue,
             ze_fence_desc_t* fence_desc,
             ze_fence_handle_t* fence) {
        fences.at(worker_idx).get(queue, fence_desc, fence);
    }

    void get(size_t worker_idx,
             ze_module_handle_t module,
             std::string kernel_name,
             ze_kernel_handle_t* kernel) {
        kernels.at(worker_idx).get(module, kernel_name, kernel);
    }

    void get(size_t worker_idx,
             ze_context_handle_t context,
             ze_device_handle_t device,
             ze_command_list_desc_t* list_desc,
             ze_command_list_handle_t* list) {
        lists.at(worker_idx).get(context, device, list_desc, list);
    }

    void get(size_t worker_idx,
             ze_context_handle_t context,
             ze_device_handle_t device,
             ze_command_queue_desc_t* queue_desc,
             ze_command_queue_handle_t* queue) {
        queues.at(worker_idx).get(context, device, queue_desc, queue);
    }

    void get(ze_context_handle_t context, ze_device_handle_t device, ze_module_handle_t* module) {
        modules.get(context, device, module);
    }

    /* push */
    void push(size_t worker_idx,
              ze_command_queue_handle_t queue,
              ze_fence_desc_t* fence_desc,
              ze_fence_handle_t* fence) {
        fences.at(worker_idx).push(queue, fence_desc, fence);
    }

    void push(size_t worker_idx,
              ze_module_handle_t module,
              std::string kernel_name,
              ze_kernel_handle_t* kernel) {
        kernels.at(worker_idx).push(module, kernel_name, kernel);
    }

    void push(size_t worker_idx,
              ze_context_handle_t context,
              ze_device_handle_t device,
              ze_command_list_desc_t* list_desc,
              ze_command_list_handle_t* list) {
        lists.at(worker_idx).push(context, device, list_desc, list);
    }

    void push(size_t worker_idx,
              ze_context_handle_t context,
              ze_device_handle_t device,
              ze_command_queue_desc_t* queue_desc,
              ze_command_queue_handle_t* queue) {
        queues.at(worker_idx).push(context, device, queue_desc, queue);
    }

private:
    std::vector<fence_cache> fences;
    std::vector<kernel_cache> kernels;
    std::vector<list_cache> lists;
    std::vector<queue_cache> queues;
    module_cache modules;
};

} // namespace ze
} // namespace ccl
