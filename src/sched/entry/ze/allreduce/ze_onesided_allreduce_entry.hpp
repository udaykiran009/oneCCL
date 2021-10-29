#pragma once

#include "common/utils/buffer.hpp"
#include "comp/comp.hpp"
#include "sched/entry/ze/ze_base_entry.hpp"

#include <atomic>
#include <sstream>

class ze_onesided_allreduce_entry : public ze_base_entry {
public:
    static constexpr const char* class_name() noexcept {
        return "ZE_1S_ALLREDUCE";
    }

    const char* name() const noexcept override {
        return class_name();
    }

    virtual std::string name_ext() const override {
        std::stringstream out;
        out << name() << " ";
        out << "size: " << cnt;
        return out.str();
    }

    ze_onesided_allreduce_entry() = delete;
    explicit ze_onesided_allreduce_entry(ccl_sched* sched,
                                         ccl_buffer send_buf,
                                         ccl_buffer recv_buf,
                                         size_t cnt,
                                         const ccl_datatype& dtype,
                                         ccl::reduction op,
                                         ccl_comm* comm,
                                         std::vector<ze_event_handle_t> wait_events = {});

    void init_ze_hook() override;
    void finalize_ze_hook() override;

    void start() override;
    void update() override;

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
    const ccl_buffer send_buf;
    const ccl_buffer recv_buf;
    void* send_buf_ptr{};
    void* recv_buf_ptr{};
    void* right_send_buf_ptr{};
    void* right_recv_buf_ptr{};
    const unsigned long cnt;
    const ccl_datatype dtype;
    const ccl::reduction op;
    const size_t buf_size_bytes;

    ze_event_handle_t empty_kernel_event{};
    ze_event_handle_t copy_from_peer_event{};
    ze_event_handle_t reduce_local_kernel_event{};

    ze_group_count_t group_count{};

    ze_kernel_handle_t main_kernel{};
    std::string main_kernel_name{};

    ze_kernel_handle_t empty_kernel{};
    std::string empty_kernel_name{ "empty_kernel" };
};
