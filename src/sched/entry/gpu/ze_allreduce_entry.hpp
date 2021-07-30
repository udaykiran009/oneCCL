#pragma once

#include "common/comm/comm.hpp"
#include "common/global/global.hpp"
#include "common/utils/buffer.hpp"
#include "comp/comp.hpp"
#include "sched/entry/entry.hpp"

#include <atomic>
#include <sstream>
#include <ze_api.h>

class ze_allreduce_entry : public sched_entry {
public:
    static constexpr const char* class_name() noexcept {
        return "ZE_ALLREDUCE";
    }

    const char* name() const noexcept override {
        return class_name();
    }

    bool is_gpu_entry() const noexcept override {
        return true;
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
    ccl_sched* const sched;
    ccl_buffer send_buf;
    ccl_buffer recv_buf;
    void* send_buf_ptr;
    void* recv_buf_ptr;
    void* right_send_buf_ptr;
    void* right_recv_buf_ptr;
    void* tmp_buf_ptr;
    const int rank;
    const int comm_size;
    const unsigned long cnt;
    const ccl_datatype dtype;
    const ccl::reduction op;
    const size_t buf_size_bytes;
    bool is_initialized;
    size_t worker_idx;

    ze_context_handle_t context;
    ze_device_handle_t device;

    ze_command_queue_desc_t comp_queue_desc;
    ze_command_queue_handle_t comp_queue;

    ze_command_list_desc_t comp_list_desc;
    ze_command_list_handle_t comp_list;

    ze_event_pool_desc_t event_pool_desc;
    ze_event_pool_handle_t event_pool;

    ze_event_handle_t copy_from_peer_event;
    ze_event_handle_t reduce_local_kernel_event;
    ze_event_handle_t copy_to_peer_event;
    ze_event_handle_t empty_kernel_event;

    ze_device_mem_alloc_desc_t device_mem_alloc_desc;

    ze_module_handle_t module;
    ze_kernel_handle_t kernel;
    ze_kernel_handle_t empty_kernel;
    std::string kernel_name;
    std::string empty_kernel_name;
    ze_group_count_t group_count;

    ze_fence_desc_t fence_desc;
    ze_fence_handle_t fence;
};
