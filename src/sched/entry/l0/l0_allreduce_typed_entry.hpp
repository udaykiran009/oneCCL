#pragma once

#include <initializer_list>
#include <atomic>

#include "sched/entry/l0/l0_entry.hpp"
#include "common/comm/l0/context/scale/ipc/ipc_ctx_impl.hpp"
#include "kernels/shared.h"

namespace native {
template <class gpu_comm_impl, ccl::group_split_type topology>
class l0_allreduce_typed_entry : public base_gpu_entry<gpu_comm_impl,
                                                       topology,
                                                       ccl::device_topology_type::ring,
                                                       ccl_coll_allreduce> {
public:
    friend class ccl_gpu_comm;
    friend class ccl_virtual_gpu_comm;

    using base = base_gpu_entry<gpu_comm_impl,
                                topology,
                                ccl::device_topology_type::ring,
                                ccl_coll_allreduce>;
    using base::parent_communicator;
    using base::comm_addr;
    using base::req;
    using base::status;
    using base::kernel_router;
    using base::get_ctx;
    using base::alloc_memory_wrap;
    using base::get_local_kernel;
    using kernel_main_typed = ring::allreduce::main_kernel;

    static constexpr const char* class_name() noexcept {
        return "L0_ALLREDUCE_TYPED";
    }

    static constexpr ccl_coll_type type() noexcept {
        return ccl_coll_allreduce;
    }

    l0_allreduce_typed_entry() = delete;
    l0_allreduce_typed_entry(
        ccl_sched* sched,
        std::shared_ptr<gpu_comm_impl> comm,
        specific_indexed_device_storage& available_devices,
        ccl_driver_context_ptr in_ctx,
        const ccl_buffer send_buf,
        ccl_buffer recv_buf,
        size_t cnt,
        const coll_param_gpu& params,
        std::shared_ptr<ccl_stream> device_stream = std::shared_ptr<ccl_stream>())
            : base(sched, comm, in_ctx, send_buf, params, device_stream) {
        recv_buf_typed_entry = recv_buf;
        cnt_entry = cnt;

        int next_rank = (comm_addr.rank + 1) % comm_addr.size;
        kernel_router = base::template create_kernel_router_for_rank<
            l0_allreduce_typed_entry<gpu_comm_impl, topology>>(
            *this, next_rank, available_devices, base::get_params());

        ENTRY_LOG_DEBUG("Init phase of current entry for ext_rank:", next_rank);

        this->set_state(gpu_entry_state::created);
    }

    ~l0_allreduce_typed_entry() {
        // TODO: remove the memory once the entry is destroyed if it's not cleared automatically
        // TODO: should we destroy handles here?
    }

    void start() override {
        ENTRY_LOG_DEBUG("Start entry, cnt ", cnt_entry);

        //Create base primitives
        base::start();

        auto& main_entry_function = get_local_kernel();

        // TODO: try to remove indirect buffer
        void* recv_buf_ptr = recv_buf_typed_entry.get_ptr();

        //create implementation specified primitives
        main_entry_function.template set_args<typename ring::allreduce::recv_buf_arg<void>,
                                              typename ring::allreduce::send_buf_size_arg>(
            recv_buf_ptr, cnt_entry);

        // Once we filled our local parameters, we go wait for another entry to set its
        // parameters so we can use them
        this->set_state(gpu_entry_state::wait_for_entry);

        //make sure, that kernel ready for launch
        this->submit_for_execution();
        status = ccl_sched_entry_status_started;
    }

    const char* name() const override {
        return class_name();
    }

    std::vector<ccl_device::device_ipc_memory_handle> get_ipc_data() override {
        ccl_device& owned_device = parent_communicator->get_device();

        auto send_buf_ptr = reinterpret_cast<void*>(base::send_buf.get_ptr());
        auto recv_buf_ptr = reinterpret_cast<void*>(recv_buf_typed_entry.get_ptr());

        std::vector<ccl_device::device_ipc_memory_handle> ret;
        ret.reserve(2);
        ret.push_back(owned_device.create_ipc_memory_handle(send_buf_ptr, get_ctx()));
        ret.push_back(owned_device.create_ipc_memory_handle(recv_buf_ptr, get_ctx()));
        return ret;
    }

    observer::invoke_params<type()> get_numa_data() override {
        observer::producer_description in_params{
            .rank = comm_addr.rank, //TODO unused
            .comm_size = comm_addr.size, //TODO unused
            .staged_buffer_elem_count = cnt_entry,
            .context = get_ctx(),
            .device = parent_communicator->get_device(),
            .immediate_list = parent_communicator->get_device().create_immediate_cmd_list(get_ctx())
        };
        // TODO: Should get_params() be a part of in_params?
        return observer::invoke_params<type()>(std::move(in_params), base::get_params());
    }

protected:
    void dump_detail(std::stringstream& str) const override {
        base::dump_detail(str);
    }

private:
    ccl_buffer recv_buf_typed_entry;
    size_t cnt_entry;

public:
    template <class left_kernel_t, class right_kernel_t>
    bool execute(left_kernel_t& left_kernel, right_kernel_t& right_kernel) {
        bool is_right_kernel_ready =
            right_kernel.template test_args<typename ring::allreduce::send_buf_arg<void>,
                                            typename ring::allreduce::recv_buf_arg<void>>();

        if (is_right_kernel_ready) {
            auto right_send_buf_arg =
                right_kernel.template get_arg<typename ring::allreduce::send_buf_arg<void>>();
            auto right_recv_buf_arg =
                right_kernel.template get_arg<typename ring::allreduce::recv_buf_arg<void>>();

            left_kernel.template set_args<typename ring::allreduce::right_send_buf_arg<void>,
                                          typename ring::allreduce::right_recv_buf_arg<void>>(
                right_send_buf_arg.second, right_recv_buf_arg.second);

            ENTRY_LOG_DEBUG("Binding arguments between kernels is complete. ",
                            "Arguments of the left kernel after binding:\n",
                            left_kernel.to_string());
        }
        return is_right_kernel_ready;
    }
};
} // namespace native
