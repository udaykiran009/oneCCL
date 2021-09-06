#pragma once

#include "common/utils/buffer.hpp"
#include "comp/comp.hpp"
#include "sched/entry/gpu/ze_base_entry.hpp"

#include <atomic>
#include <sstream>

class ze_a2a_allreduce_entry : public ze_base_entry {
public:
    static constexpr const char* class_name() noexcept {
        return "ZE_A2A_ALLREDUCE";
    }

    const char* name() const noexcept override {
        return class_name();
    }

    ze_a2a_allreduce_entry() = delete;
    explicit ze_a2a_allreduce_entry(ccl_sched* sched,
                                    ccl_buffer send_buf,
                                    ccl_buffer recv_buf,
                                    size_t cnt,
                                    const ccl_datatype& dtype,
                                    ccl::reduction op,
                                    ccl_comm* comm);
    ~ze_a2a_allreduce_entry();

    void init();
    void start() override;
    void update() override;
    void finalize();

    bool is_strict_order_satisfied() override {
        return (status >= ccl_sched_entry_status_complete);
    }

protected:
    void dump_detail(std::stringstream& str) const override {
        ccl_logger::format(str,
                           "dt ",
                           ccl::global_data::get().dtypes->name(dtype),
                           ", buf_cnt ",
                           buf_count,
                           ", send_buf ",
                           send_buf,
                           ", recv_buf ",
                           recv_buf,
                           ", op ",
                           ccl_reduction_to_str(op),
                           ", comm_id ",
                           sched->get_comm_id(),
                           ", context ",
                           context,
                           "\n");
    }

private:
    static constexpr size_t event_group_count{ 3 }; // copy + kernels + copy phases

    const ccl_buffer send_buf;
    const ccl_buffer recv_buf;
    const ccl_datatype dtype;
    const ccl::reduction op;
    const size_t buf_count;
    const size_t buf_bytes;
    const int peer_count;

    void* tmp_buf{};
    size_t tmp_buf_bytes{};
    std::vector<ze_event_handle_t> pre_copy_events;
    std::vector<ze_event_handle_t> post_copy_events;
    ze_event_handle_t barrier_event{};

    ze_module_handle_t module{};
    std::vector<ze_kernel> kernels;
    std::vector<ze_event_handle_t> kernel_events;

    void kernel_init(size_t main_block_count, size_t block_count);
};
