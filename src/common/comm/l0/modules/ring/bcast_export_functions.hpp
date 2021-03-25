#pragma once
#include "common/comm/l0/modules/kernel_functions.hpp"

namespace native {

namespace ring {

namespace bcast {

/**
 * Common args for all kernel types
 */

using buf_size_arg = arg<main_kernel_args::args_start_index, size_t>;
using buf_size_arg_type = typename buf_size_arg::arg_type;

template <class native_t>
using buf_arg = thread_exchangable_arg<main_kernel_args::args_start_index + 1, native_t*>;

using income_data_flag_arg = external_arg<main_kernel_args::args_start_index + 2, int*>;
using income_data_flag_arg_type = typename income_data_flag_arg::arg_type;

using ready_to_recv_flag_arg = external_arg<main_kernel_args::args_start_index + 3, int*>;
using ready_to_recv_flag_arg_type = typename ready_to_recv_flag_arg::arg_type;

using local_barrier_flag_arg = arg<main_kernel_args::args_start_index + 4, int*>;
using local_barrier_flag_arg_type = typename local_barrier_flag_arg::arg_type;

template <class native_t>
using right_buf_arg = thread_exchangable_arg<main_kernel_args::args_start_index + 5, native_t*>;

using right_income_data_flag_arg =
    thread_exchangable_arg<main_kernel_args::args_start_index + 6, int*>;
using right_income_data_flag_arg_type = typename right_income_data_flag_arg::arg_type;

using right_ready_to_recv_flag_arg =
    thread_exchangable_arg<main_kernel_args::args_start_index + 7, int*>;
using right_ready_to_recv_flag_arg_type = typename right_ready_to_recv_flag_arg::arg_type;

using root_arg = arg<main_kernel_args::args_start_index + 8, size_t>;
using root_arg_type = typename root_arg::arg_type;

// IMPORTANT: the number and types of arguments must be the same in all classes,
// excluding arguments specific for numa/scaleout etc.
template <class kernel_params>
struct main_kernel : public execution_kernel<main_kernel<kernel_params>,
                                             buf_size_arg,
                                             buf_arg<typename kernel_params::native_type>,
                                             income_data_flag_arg,
                                             ready_to_recv_flag_arg,
                                             local_barrier_flag_arg,
                                             right_buf_arg<typename kernel_params::native_type>,
                                             right_income_data_flag_arg,
                                             right_ready_to_recv_flag_arg,
                                             root_arg> {
    using processing_type = typename kernel_params::native_type;

    static constexpr const char* specific_name() {
        return "bcast_execution";
    }

    using common_entry_buf_size_arg = buf_size_arg;
    using common_entry_buf_arg = buf_arg<processing_type>;

    using base = execution_kernel<main_kernel<kernel_params>,
                                  buf_size_arg,
                                  buf_arg<processing_type>,
                                  income_data_flag_arg,
                                  ready_to_recv_flag_arg,
                                  local_barrier_flag_arg,
                                  right_buf_arg<processing_type>,
                                  right_income_data_flag_arg,
                                  right_ready_to_recv_flag_arg,
                                  root_arg>;
};

template <class kernel_params>
struct numa_kernel
        : public execution_kernel<numa_kernel<kernel_params>,
                                  buf_size_arg,
                                  buf_arg<typename kernel_params::native_type>,
                                  income_data_flag_arg,
                                  ready_to_recv_flag_arg,
                                  local_barrier_flag_arg,
                                  right_buf_arg<typename kernel_params::native_type>,
                                  right_income_data_flag_arg,
                                  right_ready_to_recv_flag_arg,
                                  root_arg,

                                  // numa-specific args
                                  permanent_arg<main_kernel_args::args_start_index + 9,
                                                typename kernel_params::native_type*>,
                                  permanent_arg<main_kernel_args::args_start_index + 10, int*>> {
    using processing_type = typename kernel_params::native_type;

    static constexpr const char* specific_name() {
        return "bcast_execution_numa";
    }

    using common_entry_buf_arg = buf_arg<processing_type>;

    // event data
    using event_prod_chunk_mem_arg =
        permanent_arg<main_kernel_args::args_start_index + 9, typename kernel_params::native_type*>;
    using event_prod_chunk_mem_arg_type = typename event_prod_chunk_mem_arg::arg_type;

    using event_prod_bytes_arg = permanent_arg<main_kernel_args::args_start_index + 10, int*>;
    using event_prod_bytes_arg_type = typename event_prod_bytes_arg::arg_type;

    using base = execution_kernel<numa_kernel<kernel_params>,
                                  buf_size_arg,
                                  buf_arg<processing_type>,
                                  income_data_flag_arg,
                                  ready_to_recv_flag_arg,
                                  local_barrier_flag_arg,
                                  right_buf_arg<processing_type>,
                                  right_income_data_flag_arg,
                                  right_ready_to_recv_flag_arg,
                                  root_arg,
                                  event_prod_chunk_mem_arg,
                                  event_prod_bytes_arg>;

    template <class ctx_params_t>
    void bind_data(const ctx_params_t& out_ctx_params) {
        // TODO not implemented
        (void)out_ctx_params;
        throw ccl::exception(std::string(__FUNCTION__) + " - not implemented for that kernel type");
    }
};

template <class kernel_params>
struct ipc_kernel : public base_ipc_kernel<ipc_kernel<kernel_params>,
                                           stub_arg<main_kernel_args::args_start_index>,
                                           buf_arg<typename kernel_params::native_type>,
                                           income_data_flag_arg,
                                           ready_to_recv_flag_arg,
                                           stub_arg<main_kernel_args::args_start_index + 4>,
                                           stub_arg<main_kernel_args::args_start_index + 5>,
                                           stub_arg<main_kernel_args::args_start_index + 6>,
                                           stub_arg<main_kernel_args::args_start_index + 7>,
                                           stub_arg<main_kernel_args::args_start_index + 8>> {
    using processing_type = typename kernel_params::native_type;

    static constexpr const char* specific_name() {
        return "ring_bcast_ipc";
    }

    using common_entry_buf_arg = buf_arg<processing_type>;

    using base = base_ipc_kernel<ipc_kernel<kernel_params>,
                                 stub_arg<main_kernel_args::args_start_index>,
                                 buf_arg<processing_type>,
                                 income_data_flag_arg,
                                 ready_to_recv_flag_arg,
                                 stub_arg<main_kernel_args::args_start_index + 4>,
                                 stub_arg<main_kernel_args::args_start_index + 5>,
                                 stub_arg<main_kernel_args::args_start_index + 6>,
                                 stub_arg<main_kernel_args::args_start_index + 7>,
                                 stub_arg<main_kernel_args::args_start_index + 8>>;

    template <class ipc_handles_t>
    void bind_data(const ipc_handles_t& ipc_handles) {
        auto recv_buf = reinterpret_cast<typename buf_arg<processing_type>::arg_type>(
            ipc_handles.at(0).get().pointer);
        this->template set_arg<buf_arg<processing_type>>(recv_buf);

        auto income_data_flag =
            reinterpret_cast<income_data_flag_arg_type>(ipc_handles.at(1).get().pointer);
        this->template set_arg<income_data_flag_arg>(income_data_flag);

        auto ready_to_recv_flag =
            reinterpret_cast<ready_to_recv_flag_arg_type>(ipc_handles.at(2).get().pointer);
        this->template set_arg<ready_to_recv_flag_arg>(ready_to_recv_flag);
    }
};

template <class kernel_params>
struct scale_out_cpu_gw_kernel
        : public execution_kernel<scale_out_cpu_gw_kernel<kernel_params>,
                                  buf_size_arg,
                                  buf_arg<typename kernel_params::native_type>,
                                  income_data_flag_arg,
                                  ready_to_recv_flag_arg,
                                  local_barrier_flag_arg,
                                  right_buf_arg<typename kernel_params::native_type>,
                                  right_income_data_flag_arg,
                                  right_ready_to_recv_flag_arg,
                                  root_arg,

                                  // scaleout-specific args
                                  permanent_arg<main_kernel_args::args_start_index + 9,
                                                typename kernel_params::native_type*>,
                                  permanent_arg<main_kernel_args::args_start_index + 10, int*>> {
    using param_t = kernel_params;
    using processing_type = typename param_t::native_type;

    static constexpr const char* specific_name() {
        return "bcast_execution_scale_out_cpu_gw";
    }

    using common_entry_buf_arg = buf_arg<processing_type>;

    // event data
    using event_prod_chunk_mem_arg =
        permanent_arg<main_kernel_args::args_start_index + 9, processing_type*>;
    using event_prod_chunk_mem_arg_type = typename event_prod_chunk_mem_arg::arg_type;

    using event_prod_bytes_arg = permanent_arg<main_kernel_args::args_start_index + 10, int*>;
    using event_prod_bytes_arg_type = typename event_prod_bytes_arg::arg_type;

    using base = execution_kernel<scale_out_cpu_gw_kernel<kernel_params>,
                                  buf_size_arg,
                                  buf_arg<processing_type>,
                                  income_data_flag_arg,
                                  ready_to_recv_flag_arg,
                                  local_barrier_flag_arg,
                                  right_buf_arg<processing_type>,
                                  right_income_data_flag_arg,
                                  right_ready_to_recv_flag_arg,
                                  root_arg,
                                  event_prod_chunk_mem_arg,
                                  event_prod_bytes_arg>;

    template <class ctx_params_t>
    void bind_data(const ctx_params_t& out_ctx_params) {
        // TODO not implemented
        (void)out_ctx_params;
        throw ccl::exception(std::string(__FUNCTION__) + " - not implemented for that kernel type");
    }
};

} // namespace bcast
} // namespace ring
} // namespace native
