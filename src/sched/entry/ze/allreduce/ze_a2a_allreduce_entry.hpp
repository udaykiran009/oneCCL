#pragma once

#include "common/utils/buffer.hpp"
#include "sched/entry/ze/ze_base_entry.hpp"

class ze_a2a_allreduce_entry : public ze_base_entry {
public:
    static constexpr const char* class_name() noexcept {
        return "ZE_A2A_ALLREDUCE";
    }

    const char* name() const noexcept override {
        return class_name();
    }

    virtual std::string name_ext() const override;

    ze_a2a_allreduce_entry() = delete;
    explicit ze_a2a_allreduce_entry(ccl_sched* sched,
                                    ccl_buffer send_buf,
                                    ccl_buffer recv_buf,
                                    size_t cnt,
                                    const ccl_datatype& dtype,
                                    ccl::reduction op,
                                    ccl_comm* comm,
                                    std::vector<ze_event_handle_t> wait_events = {},
                                    size_t send_buf_idx = 0,
                                    size_t recv_buf_idx = 1,
                                    size_t peer_buf_offset = 0);

    void init_ze_hook() override;

    void start() override;
    void update() override;

protected:
    void dump_detail(std::stringstream& str) const override;

private:
    static constexpr size_t event_group_count{ 3 }; // copy + kernel + copy

    const ccl_buffer send_buf;
    const ccl_buffer recv_buf;
    const size_t cnt;
    const ccl_datatype dtype;
    const ccl::reduction op;

    const size_t send_buf_idx;
    const size_t recv_buf_idx;
    const size_t peer_buf_offset;

    const int peer_count;

    std::vector<ze_event_handle_t> pre_copy_events;
    std::vector<ze_event_handle_t> post_copy_events;
    ze_event_handle_t barrier_event{};

    std::vector<ze_kernel> kernels;
    std::vector<ze_event_handle_t> kernel_events;

    bool skip_entry{};
};
