#pragma once
#include "common/comm/l0/modules/kernel_functions.hpp"

namespace native {

namespace ring {

namespace allreduce {

/**
 * Common args for all kernel types
 */

// own
template <class native_t>
using send_buf_arg = arg<main_kernel_args::args_start_index, native_t*>;

template <class native_t>
using recv_buf_arg = arg<main_kernel_args::args_start_index + 1, native_t*>;

template <class native_t>
using right_send_buf_arg = arg<main_kernel_args::args_start_index + 2, native_t*>;

template <class native_t>
using right_recv_buf_arg = arg<main_kernel_args::args_start_index + 3, native_t*>;

// IMPORTANT: the number and types of arguments must be the same in all classes,
// excluding arguments specific for numa/scaleout etc.
struct main_kernel : public execution_kernel<main_kernel,
                                             send_buf_arg<void>,
                                             recv_buf_arg<void>,
                                             right_send_buf_arg<void>,
                                             right_recv_buf_arg<void>> {
    using processing_type = void;

    static constexpr const char* specific_name() {
        return "allreduce_execution";
    }

    using common_entry_buf_arg = send_buf_arg<processing_type>;

    using base = execution_kernel<main_kernel,
                                  send_buf_arg<void>,
                                  recv_buf_arg<void>,
                                  right_send_buf_arg<void>,
                                  right_recv_buf_arg<void>>;

    using base::base;
};

struct numa_kernel : public execution_kernel<numa_kernel,
                                             send_buf_arg<void>,
                                             recv_buf_arg<void>,
                                             right_send_buf_arg<void>,
                                             right_recv_buf_arg<void>> {
    using processing_type = void;

    static constexpr const char* specific_name() {
        return "allreduce_execution_numa";
    }

    using common_entry_buf_arg = send_buf_arg<processing_type>;

    using base = execution_kernel<numa_kernel,
                                  send_buf_arg<void>,
                                  recv_buf_arg<void>,
                                  right_send_buf_arg<void>,
                                  right_recv_buf_arg<void>>;

    template <class ctx_params_t>
    void bind_data(const ctx_params_t& out_ctx_params) {
        // TODO not implemented
        (void)out_ctx_params;
        throw ccl::exception(std::string(__FUNCTION__) + " - not implemented for that kernel type");
    }

    using base::base;
};

struct ipc_kernel : public base_ipc_kernel<ipc_kernel,
                                           send_buf_arg<void>,
                                           recv_buf_arg<void>,
                                           stub_arg<main_kernel_args::args_start_index + 2>,
                                           stub_arg<main_kernel_args::args_start_index + 3>> {
    using processing_type = void;

    using common_entry_buf_arg = send_buf_arg<processing_type>;

    static constexpr const char* specific_name() {
        return "ring_allreduce_ipc";
    }

    using base = base_ipc_kernel<ipc_kernel,
                                 send_buf_arg<processing_type>,
                                 recv_buf_arg<processing_type>,
                                 stub_arg<main_kernel_args::args_start_index + 2>,
                                 stub_arg<main_kernel_args::args_start_index + 3>>;

    template <class ipc_handles_t>
    void bind_data(const ipc_handles_t& ipc_handles) {
        auto send_buf = reinterpret_cast<typename send_buf_arg<processing_type>::arg_type>(
            ipc_handles.at(0).get().pointer);
        this->template set_arg<send_buf_arg<processing_type>>(send_buf);

        auto recv_buf = reinterpret_cast<typename recv_buf_arg<processing_type>::arg_type>(
            ipc_handles.at(1).get().pointer);
        this->template set_arg<recv_buf_arg<processing_type>>(recv_buf);
    }

    using base::base;
};

struct scale_out_cpu_gw_kernel : public execution_kernel<scale_out_cpu_gw_kernel,
                                                         send_buf_arg<void>,
                                                         recv_buf_arg<void>,
                                                         right_send_buf_arg<void>,
                                                         right_recv_buf_arg<void>> {
    using processing_type = void;

    static constexpr const char* specific_name() {
        return "allreduce_execution_scale_out_cpu_gw";
    }

    using common_entry_buf_arg = send_buf_arg<processing_type>;

    using base = execution_kernel<scale_out_cpu_gw_kernel,
                                  send_buf_arg<processing_type>,
                                  recv_buf_arg<processing_type>,
                                  right_send_buf_arg<processing_type>,
                                  right_recv_buf_arg<processing_type>>;

    template <class ctx_params_t>
    void bind_data(const ctx_params_t& out_ctx_params) {
        // TODO not implemented
        (void)out_ctx_params;
        throw ccl::exception(std::string(__FUNCTION__) + " - not implemented for that kernel type");
    }

    using base::base;
};

} // namespace allreduce
} // namespace ring
} // namespace native
