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
    bool is_gpu_entry() const noexcept override {
        return true;
    }

    ze_base_entry() = delete;
    ze_base_entry(const ze_base_entry &) = delete;
    virtual ~ze_base_entry(){};

protected:
    explicit ze_base_entry(ccl_sched *sched, uint32_t add_event_count = 0);

    void init(init_mode ze_init_mode);
    virtual void start() override;
    virtual void update() override;
    void finalize();

    ze_command_list_handle_t get_copy_list();

    void init_primitives(cmd_primitives &cmd_primitives);
    void get_copy_primitives(const ze_queue_properties_t &queue_props,
                             cmd_primitives &copy_primitives,
                             init_mode ze_init_mode);
    void get_comp_primitives(const ze_queue_properties_t &queue_props,
                             cmd_primitives &comp_primitives);

    ccl_sched *const sched;
    const uint32_t add_event_count;
    int rank{};
    int comm_size{};
    size_t worker_idx{};

    bool is_initialized{};

    ze_device_handle_t device{};
    ze_context_handle_t context{};

    cmd_primitives comp_primitives{};
    cmd_primitives copy_primitives{};

    ze_event_pool_desc_t event_pool_desc{};
    ze_event_pool_handle_t event_pool{};

    ze_event_handle_t entry_event{};
};
