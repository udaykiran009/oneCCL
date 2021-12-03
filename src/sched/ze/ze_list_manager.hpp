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
    bool is_copy{};
};

struct queue_info {
    queue_info() = default;

    explicit operator bool() const {
        return queue != nullptr;
    }

    ze_command_queue_handle_t queue{};
    ze_command_queue_desc_t desc{};
    bool is_copy{};
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

    ze_command_list_handle_t get_comp_list();
    ze_command_list_handle_t get_copy_list();

    void clear();
    void reset_execution_state();

    bool can_use_copy_queue() const;
    bool is_executed() const;

private:
    const ze_device_handle_t device;
    const ze_context_handle_t context;

    static constexpr ssize_t worker_idx = 0;

    list_info comp_list{};
    list_info copy_list{};

    ze_queue_properties_t queue_props;

    queue_info comp_queue{};
    queue_info copy_queue{};

    // used to execute in order of use
    std::list<std::pair<queue_info&, list_info&>> exec_list;

    bool use_copy_queue{};
    bool executed{};

    // not thread safe. It is supposed to be called from a single threaded builder
    queue_info create_queue(init_mode mode);
    void free_queue(queue_info& info);

    list_info create_list(const queue_info& info);
    void free_list(list_info& info);

    void execute_list(queue_info& queue, list_info& list);
};

} // namespace ze
} // namespace ccl
