#pragma once

#include <numeric>
#include "common/utils/buffer.hpp"
#include "comp/comp.hpp"
#include "sched/entry/ze/ze_base_entry.hpp"

class ze_a2a_reduce_scatter_entry : public ze_base_entry {
public:
    static constexpr const char* class_name() noexcept {
        return "ZE_A2A_REDUCE_SCATTER";
    }

    const char* name() const noexcept override {
        return class_name();
    }

    virtual std::string name_ext() const override {
        std::stringstream out;
        out << name() << " ";
        out << "size: " << std::accumulate(recv_counts.begin(), recv_counts.end(), 0);
        return out.str();
    }

    ze_a2a_reduce_scatter_entry() = delete;
    explicit ze_a2a_reduce_scatter_entry(ccl_sched* sched,
                                         ccl_buffer send_buf,
                                         ccl_buffer recv_buf,
                                         const size_t* recv_counts,
                                         const ccl_datatype& dtype,
                                         ccl::reduction op,
                                         ccl_comm* comm,
                                         std::vector<ze_event_handle_t> wait_events = {},
                                         size_t peer_buf_idx = 0);

    void init_ze_hook() override;

    void update() override;

    static void fill_list(const ze_base_entry* entry,
                          void* send_buf,
                          void* recv_buf,
                          const std::vector<ccl_buffer>& peer_send_bufs,
                          int peer_count,
                          int comm_rank,
                          size_t block_count,
                          size_t offset_bytes,
                          std::vector<ze_event_handle_t>& copy_events,
                          std::vector<ze_kernel>& kernels,
                          std::vector<ze_event_handle_t>& kernel_events,
                          ze_event_handle_t& barrier_event,
                          const ccl_datatype& dtype,
                          ze_module_handle_t module,
                          ze_device_handle_t device,
                          ze_context_handle_t context,
                          ccl::reduction op,
                          size_t worker_idx);

private:
    static constexpr size_t event_group_count{ 3 }; // copy + kernel + copy

    const ccl_buffer send_buf;
    const ccl_buffer recv_buf;
    const ccl_datatype dtype;
    const ccl::reduction op;
    const std::vector<size_t> recv_counts;
    const size_t peer_buf_idx;
    const int peer_count;

    std::vector<ze_event_handle_t> pre_copy_events;
    std::vector<ze_event_handle_t> post_copy_events;
    ze_event_handle_t barrier_event{};

    std::vector<ze_kernel> kernels;
    std::vector<ze_event_handle_t> kernel_events;

    static void kernel_init(size_t offset_bytes,
                            size_t block_count,
                            void* send_buf,
                            void* base_ptr,
                            int peer_count,
                            const ccl_datatype& dtype,
                            int comm_rank,
                            std::vector<ze_kernel>& kernels,
                            ze_module_handle_t module,
                            ze_device_handle_t device,
                            ze_context_handle_t context,
                            ccl::reduction op,
                            size_t worker_idx);
};
