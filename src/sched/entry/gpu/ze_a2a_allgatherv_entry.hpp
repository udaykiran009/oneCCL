#pragma once

#include "common/utils/buffer.hpp"
#include "sched/entry/gpu/ze_base_entry.hpp"

class ze_a2a_allgatherv_entry : public ze_base_entry {
public:
    static constexpr const char* class_name() noexcept {
        return "ZE_ALLGATHERV";
    }

    const char* name() const override {
        return class_name();
    }

    bool is_strict_order_satisfied() override {
        return (status >= ccl_sched_entry_status_complete);
    }

    explicit ze_a2a_allgatherv_entry(ccl_sched* sched,
                                     ccl_buffer send_buf,
                                     size_t send_count,
                                     ccl_buffer recv_buf,
                                     const size_t* recv_counts,
                                     const ccl_datatype& dtype,
                                     ccl_comm* comm,
                                     size_t peer_buf_idx = 0);
    ~ze_a2a_allgatherv_entry();

    void init();
    void start() override;
    void update() override;
    void finalize();

    static void fill_list(ze_command_list_handle_t list,
                          void* send_buf,
                          void* recv_buf,
                          const std::vector<ccl_buffer>& peer_recv_bufs,
                          int peer_count,
                          size_t copy_bytes,
                          size_t offset_bytes,
                          bool is_inplace,
                          std::vector<ze_event_handle_t>& copy_events,
                          ze_event_handle_t wait_event = nullptr);

private:
    static constexpr size_t event_group_count{ 1 }; // copy phase

    const ccl_buffer send_buf;
    const size_t send_bytes;
    const ccl_buffer recv_buf;
    const std::vector<size_t> recv_counts;
    const ccl_datatype dtype;
    const size_t peer_buf_idx;
    const int peer_count;

    std::vector<ze_event_handle_t> copy_events;
};
