#pragma once

#include "common/utils/buffer.hpp"
#include "comp/comp.hpp"
#include "sched/entry/gpu/ze_base_entry.hpp"

#include <atomic>
#include <sstream>

class ze_allreduce_entry : public ze_base_entry {
public:
    static constexpr const char* class_name() noexcept {
        return "ZE_ALLREDUCE";
    }

    const char* name() const noexcept override {
        return class_name();
    }

    ze_allreduce_entry() = delete;
    explicit ze_allreduce_entry(ccl_sched* sched,
                                ccl_buffer send_buf,
                                ccl_buffer recv_buf,
                                size_t cnt,
                                const ccl_datatype& dtype,
                                ccl::reduction op,
                                ccl_comm* comm);
    ~ze_allreduce_entry();

    void init();
    void start() override;
    void update() override;
    void finalize();

    void reset_sync_objects();

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

    const ccl_buffer send_buf;
    const ccl_buffer recv_buf;
    void* send_buf_ptr{};
    void* recv_buf_ptr{};
    void* right_send_buf_ptr{};
    void* right_recv_buf_ptr{};
    void* tmp_buf_ptr{};
    const unsigned long cnt;
    const ccl_datatype dtype;
    const ccl::reduction op;
    const size_t buf_size_bytes;
    const ccl_comm* comm;

    ze_event_handle_t empty_kernel_event{};
    ze_event_handle_t copy_from_peer_event{};
    ze_event_handle_t reduce_local_kernel_event{};

    ze_module_handle_t module{};

    ze_group_count_t group_count{};

    ze_kernel_handle_t main_kernel{};
    std::string main_kernel_name{};

    ze_kernel_handle_t empty_kernel{};
    std::string empty_kernel_name{ "empty_kernel" };

    ze_device_mem_alloc_desc_t device_mem_alloc_desc;
};
