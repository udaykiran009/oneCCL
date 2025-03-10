#pragma once

#include "common/utils/buffer.hpp"
#include "sched/entry/ze/ze_base_entry.hpp"

class ze_a2a_allgatherv_entry : public ze_base_entry {
public:
    static constexpr const char* class_name() noexcept {
        return "ZE_A2A_ALLGATHERV";
    }

    const char* name() const override {
        return class_name();
    }

    virtual std::string name_ext() const override;

    explicit ze_a2a_allgatherv_entry(ccl_sched* sched,
                                     ccl_buffer send_buf,
                                     size_t send_count,
                                     ccl_buffer recv_buf,
                                     const size_t* recv_counts,
                                     const ccl_datatype& dtype,
                                     ccl_comm* comm,
                                     std::vector<ze_event_handle_t> wait_events = {},
                                     size_t peer_buf_idx = 0,
                                     size_t peer_buf_offset = 0);

    void init_ze_hook() override;

    void update() override;

    static void fill_list(const ze_base_entry* entry,
                          int comm_rank,
                          void* send_buf,
                          void* recv_buf,
                          const std::vector<ccl_buffer>& peer_recv_bufs,
                          int peer_count,
                          size_t copy_bytes,
                          const ccl_datatype& dtype,
                          size_t rank_buf_offset,
                          bool is_inplace,
                          std::vector<ze_event_handle_t>& copy_events,
                          ze_event_handle_t wait_event = nullptr,
                          size_t peer_buf_offset = 0);

protected:
    void dump_detail(std::stringstream& str) const override;

private:
    static constexpr size_t event_group_count{ 1 }; // copy phase

    const ccl_buffer send_buf;
    const size_t send_count;
    const ccl_buffer recv_buf;
    const std::vector<size_t> recv_counts;
    const ccl_datatype dtype;
    const size_t peer_buf_idx;
    const size_t peer_buf_offset;
    const int peer_count;

    std::vector<ze_event_handle_t> copy_events;
};
