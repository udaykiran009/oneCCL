#pragma once

#include "common/utils/buffer.hpp"
#include "comp/comp.hpp"
#include "sched/entry/gpu/ze_base_entry.hpp"

class ze_ring_allreduce_entry : public ze_base_entry {
public:
    static constexpr const char* class_name() noexcept {
        return "ZE_RING_ALLREDUCE";
    }

    const char* name() const noexcept override {
        return class_name();
    }

    ze_ring_allreduce_entry() = delete;
    explicit ze_ring_allreduce_entry(ccl_sched* sched,
                                     ccl_buffer send_buf,
                                     ccl_buffer recv_buf,
                                     ccl_buffer tmp_buf,
                                     size_t cnt,
                                     const ccl_datatype& dtype,
                                     ccl::reduction op,
                                     ccl_comm* comm);
    ~ze_ring_allreduce_entry();

    void init();
    void start() override;
    void update_multi_ranks();
    void update() override;
    void finalize();
    void reset_fields();

    bool is_strict_order_satisfied() override {
        return (status >= ccl_sched_entry_status_complete);
    }

protected:
    void dump_detail(std::stringstream& str) const override {
        ccl_logger::format(str,
                           "dt ",
                           ccl::global_data::get().dtypes->name(dtype),
                           ", cnt ",
                           cnt,
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
    static constexpr uint32_t local_events_count{ 3 };
    static constexpr uint32_t op_id_offset{ 16 };

    const ccl_buffer send_buf;
    const ccl_buffer recv_buf;
    const ccl_buffer tmp_buf;
    void* send_buf_ptr{};
    void* recv_buf_ptr{};
    void* tmp_buf_ptr{};
    void* right_recv_buf_ptr{};
    void* right_tmp_buf_ptr{};
    const unsigned long cnt;
    const ccl_datatype dtype;
    const ccl::reduction op;

    int iter_idx{};
    const int stage_iter_count;
    const int total_iter_count;

    int left_peer{};
    int right_peer{};

    bool is_rs_completed{};
    bool is_ag_completed{};

    /* atl */

    std::vector<atl_req_t> recv_reqs{};
    std::vector<atl_req_t> send_reqs{};

    std::vector<uint64_t> send_tags;
    std::vector<uint64_t> recv_tags;

    std::vector<int> sync_send_flags;
    std::vector<int> sync_recv_flags;

    std::vector<bool> rs_sync_sent;
    std::vector<bool> ag_sync_sent;

    void atl_ops_init();
    void atl_ops_finalize();
    void send_sync_flag(int idx);
    void recv_sync_flag(int idx);
    void validate_sync_flags(int limit);
    bool check_atl_req(atl_req_t* req);
    void reset_atl_reqs();

    /* gpu */

    static constexpr size_t event_group_count =
        6; // (rs_copy + rs_reduce + ag_copy) x (signal + wait)
    std::vector<ze_event_handle_t> rs_copy_signal_events;
    std::vector<ze_event_handle_t> rs_copy_wait_events;
    std::vector<ze_event_handle_t> rs_reduce_signal_events;
    std::vector<ze_event_handle_t> rs_reduce_wait_events;
    std::vector<ze_event_handle_t> ag_copy_signal_events;
    std::vector<ze_event_handle_t> ag_copy_wait_events;
    std::vector<ze_kernel> kernels;

    std::vector<bool> rs_copy_started;
    std::vector<bool> rs_reduce_started;
    std::vector<bool> ag_copy_started;

    void gpu_init();
    void gpu_finalize();
};
