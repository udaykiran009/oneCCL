#pragma once

#include "common/utils/buffer.hpp"
#include "comp/comp.hpp"
#include "sched/entry/ze/ze_base_entry.hpp"

#include <atomic>
#include <sstream>

class ze_onesided_reduce_entry : public ze_base_entry {
public:
    static constexpr const char* class_name() noexcept {
        return "ZE_1S_REDUCE";
    }

    const char* name() const noexcept override {
        return class_name();
    }

    virtual std::string name_ext() const override;

    ze_onesided_reduce_entry() = delete;
    explicit ze_onesided_reduce_entry(ccl_sched* sched,
                                      ccl_buffer send_buf,
                                      ccl_buffer recv_buf,
                                      size_t cnt,
                                      const ccl_datatype& dtype,
                                      ccl::reduction op,
                                      int root,
                                      ccl_comm* comm,
                                      std::vector<ze_event_handle_t> wait_events = {},
                                      size_t peer_buf_offset = 0);

    void init_ze_hook() override;
    void finalize_ze_hook() override;

    void start() override;
    void update() override;

protected:
    void dump_detail(std::stringstream& str) const override;

private:
    ccl_buffer send_buf;
    ccl_buffer recv_buf;
    void* send_buf_ptr;
    void* recv_buf_ptr;
    void* right_send_buf_ptr;
    const unsigned long cnt;
    const ccl_datatype dtype;
    const ccl::reduction op;
    int root;
    const size_t buf_size_bytes;
    const size_t peer_buf_offset_bytes;

    ze_event_handle_t empty_kernel_event;
    ze_event_handle_t copy_from_peer_event;

    ze_group_count_t group_count;

    ze_kernel_handle_t main_kernel;
    std::string main_kernel_name;

    ze_kernel_handle_t empty_kernel;
    std::string empty_kernel_name;

    bool skip_entry{};
};
