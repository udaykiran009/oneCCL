#pragma once

#include "sched/entry/ze/ze_primitives.hpp"

namespace ccl {
namespace ze {

struct list_info {
    list_info() = default;

    explicit operator bool() const {
        return list != nullptr;
    }

    ze_command_list_handle_t list{};
    ze_command_list_desc_t desc{};
    bool is_closed{};
    ssize_t worker_idx{ -1 };
};

struct queue_info {
    queue_info() = default;

    explicit operator bool() const {
        return queue != nullptr;
    }

    ze_command_queue_handle_t queue{};
    ze_command_queue_desc_t desc{};
    ssize_t worker_idx{ -1 };
};

class list_manager {
public:
    list_manager() = delete;
    explicit list_manager(ze_device_handle_t device, ze_context_handle_t context);
    explicit list_manager(const ccl_stream* stream);
    list_manager(const list_manager&) = delete;
    explicit list_manager(list_manager&&) = default;
    ~list_manager();

    void execute();

    ze_command_list_handle_t get_comp_list(size_t worker_idx = 0);
    ze_command_list_handle_t get_copy_list(size_t worker_idx = 0);

    void clear();

    bool can_use_copy_queue() const;

private:
    const ze_device_handle_t device;
    const ze_context_handle_t context;

    list_info comp_list{};
    list_info copy_list{};

    ze_queue_properties_t queue_props;

    queue_info comp_queue{};
    queue_info copy_queue{};

    bool use_copy_queue{};

    // not thread safe. It is supposed to be called from a single threaded builder
    queue_info create_queue(init_mode mode, size_t worker_idx);
    void free_queue(queue_info& info);

    list_info create_list(const queue_info& info, size_t worker_idx);
    void free_list(list_info& info);

    void execute_list(queue_info& queue, list_info& list);
};

} // namespace ze
} // namespace ccl
