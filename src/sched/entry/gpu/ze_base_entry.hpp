#pragma once

#include "common/comm/comm.hpp"
#include "common/global/global.hpp"
#include "sched/sched.hpp"
#include "sched/entry/entry.hpp"

#include <ze_api.h>

using namespace ccl::ze;

struct cmd_primitives {
    ze_command_queue_handle_t queue{};
    ze_command_queue_desc_t queue_desc{ default_cmd_queue_desc };
    ze_command_list_handle_t list{};
    ze_command_list_desc_t list_desc{ default_cmd_list_desc };
};

class ze_base_entry : public sched_entry {
public:
    ze_base_entry() = delete;
    ze_base_entry(const ze_base_entry &) = delete;
    virtual ~ze_base_entry(){};

protected:
    explicit ze_base_entry(ccl_sched *sched,
                           ccl_comm *comm = nullptr,
                           uint32_t add_event_count = 0);

    void init(init_mode mode);
    virtual void start() override;
    virtual void update() override;
    void finalize();

    ze_command_list_handle_t get_copy_list();

    void init_primitives(cmd_primitives &cmd_primitives);
    void get_copy_primitives(const ze_queue_properties_t &queue_props,
                             cmd_primitives &copy_primitives,
                             init_mode mode);
    void get_comp_primitives(const ze_queue_properties_t &queue_props,
                             cmd_primitives &comp_primitives);

    ze_event_handle_t create_event();
    static bool is_event_completed(ze_event_handle_t event);
    void reset_events();
    void destroy_events();

    void close_lists();

    ccl_sched *const sched;

    ccl_comm *comm{};
    int comm_rank{};
    int comm_size{};

    size_t worker_idx{};

    bool is_initialized{};

    ze_module_handle_t module{};

    ze_device_handle_t device{};
    ze_context_handle_t context{};

    cmd_primitives comp_primitives{};
    cmd_primitives copy_primitives{};

    ze_event_handle_t entry_event{};

private:
    uint32_t event_counter{};
    ze_event_pool_desc_t event_pool_desc{};
    ze_event_pool_handle_t event_pool{};
    std::vector<ze_event_handle_t> events;

    bool is_queue_completed(ze_command_queue_handle_t queue);
};

class ze_kernel {
public:
    ze_kernel(ze_module_handle_t module, const std::string &kernel_name, size_t worker_idx = 0);
    ze_kernel(const ze_kernel &) = delete;
    ze_kernel(ze_kernel &&other) noexcept;
    ~ze_kernel();

    void set_args(ze_kernel_args_t kernel_args);
    void calculate_group_size(size_t elem_count);
    ze_kernel_handle_t get_kernel() const;
    const ze_group_count_t *get_group_count() const;

private:
    ze_module_handle_t module{};
    std::string kernel_name{};
    size_t worker_idx{};
    ze_group_count_t group_count{};
    ze_group_size_t group_size{};
    ze_kernel_handle_t kernel{};
};
