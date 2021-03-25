#pragma once
#include "common/comm/l0/modules/kernel_functions.hpp"

namespace native {

namespace ring {

namespace allreduce {

/**
 * Common args for all kernel types
 */

// own
using send_buf_size_arg = arg<main_kernel_args::args_start_index, size_t>;
using send_buf_size_arg_type = typename send_buf_size_arg::arg_type;

template <class native_t>
using send_buf_arg = arg<main_kernel_args::args_start_index + 1, native_t*>;

template <class native_t>
using recv_buf_arg = arg<main_kernel_args::args_start_index + 2, native_t*>;

template <class native_t>
using tmp_recv_buf_arg = external_arg<main_kernel_args::args_start_index + 3, native_t*>;

using income_data_flag_arg = external_arg<main_kernel_args::args_start_index + 4, int*>;
using income_data_flag_arg_type = typename income_data_flag_arg::arg_type;

using ready_to_recv_flag_arg = external_arg<main_kernel_args::args_start_index + 5, int*>;
using ready_to_recv_flag_arg_type = typename ready_to_recv_flag_arg::arg_type;

using local_barrier_flag_arg = arg<main_kernel_args::args_start_index + 6, int*>;
using local_barrier_flag_arg_type = typename local_barrier_flag_arg::arg_type;

// right
template <class native_t>
using right_tmp_recv_buf_arg =
    thread_exchangable_arg<main_kernel_args::args_start_index + 7, native_t*>;

using right_income_data_flag_arg =
    thread_exchangable_arg<main_kernel_args::args_start_index + 8, int*>;

using right_ready_to_recv_flag_arg =
    thread_exchangable_arg<main_kernel_args::args_start_index + 9, int*>;

// IMPORTANT: the number and types of arguments must be the same in all classes,
// excluding arguments specific for numa/scaleout etc.
template <class kernel_params>
struct main_kernel
        : public execution_kernel<main_kernel<kernel_params>,
                                  send_buf_size_arg,
                                  send_buf_arg<typename kernel_params::native_type>,
                                  recv_buf_arg<typename kernel_params::native_type>,
                                  tmp_recv_buf_arg<typename kernel_params::native_type>,
                                  income_data_flag_arg,
                                  ready_to_recv_flag_arg,
                                  local_barrier_flag_arg,
                                  right_tmp_recv_buf_arg<typename kernel_params::native_type>,
                                  right_income_data_flag_arg,
                                  right_ready_to_recv_flag_arg> {
    using param_t = kernel_params;
    using processing_type = typename kernel_params::native_type;

    static constexpr const char* specific_name() {
        return "allreduce_execution";
    }

    using common_entry_buf_size_arg = send_buf_size_arg;
    using common_entry_buf_arg = send_buf_arg<processing_type>;

    using base = execution_kernel<main_kernel<kernel_params>,
                                  send_buf_size_arg,
                                  send_buf_arg<processing_type>,
                                  recv_buf_arg<processing_type>,
                                  tmp_recv_buf_arg<processing_type>,
                                  income_data_flag_arg,
                                  ready_to_recv_flag_arg,
                                  local_barrier_flag_arg,
                                  right_tmp_recv_buf_arg<processing_type>,
                                  right_income_data_flag_arg,
                                  right_ready_to_recv_flag_arg>;
};

template <class kernel_params>
struct numa_kernel : public execution_kernel<
                         numa_kernel<kernel_params>,
                         send_buf_size_arg,
                         send_buf_arg<typename kernel_params::native_type>,
                         recv_buf_arg<typename kernel_params::native_type>,
                         tmp_recv_buf_arg<typename kernel_params::native_type>,
                         income_data_flag_arg,
                         ready_to_recv_flag_arg,
                         local_barrier_flag_arg,
                         right_tmp_recv_buf_arg<typename kernel_params::native_type>,
                         right_income_data_flag_arg,
                         right_ready_to_recv_flag_arg,

                         // numa-specific args
                         permanent_arg<main_kernel_args::args_start_index + 10,
                                       typename kernel_params::native_type*>,
                         permanent_arg<main_kernel_args::args_start_index + 11, uint64_t*>,
                         permanent_arg<main_kernel_args::args_start_index + 12, uint64_t*>,
                         permanent_arg<main_kernel_args::args_start_index + 13,
                                       typename kernel_params::native_type*>,
                         permanent_arg<main_kernel_args::args_start_index + 14, uint64_t*>> {
    using param_t = kernel_params;
    using processing_type = typename kernel_params::native_type;

    static constexpr const char* specific_name() {
        return "allreduce_execution_numa";
    }

    using common_entry_buf_size_arg = send_buf_size_arg;
    using common_entry_buf_arg = send_buf_arg<processing_type>;

    // event data
    using event_prod_chunk_mem_arg =
        permanent_arg<main_kernel_args::args_start_index + 10, processing_type*>;
    using event_prod_chunk_mem_arg_type = typename event_prod_chunk_mem_arg::arg_type;

    using event_prod_bytes_arg = permanent_arg<main_kernel_args::args_start_index + 11, uint64_t*>;
    using event_prod_bytes_arg_type = typename event_prod_bytes_arg::arg_type;

    using event_consumed_bytes_offset_arg =
        permanent_arg<main_kernel_args::args_start_index + 12, uint64_t*>;
    using event_consumed_bytes_offset_arg_type = typename event_consumed_bytes_offset_arg::arg_type;

    using event_consumed_chunk_mem_arg =
        permanent_arg<main_kernel_args::args_start_index + 13, processing_type*>;
    using event_consumed_chunk_mem_arg_type = typename event_consumed_chunk_mem_arg::arg_type;

    using event_consumed_bytes_arg =
        permanent_arg<main_kernel_args::args_start_index + 14, uint64_t*>;
    using event_consumed_bytes_arg_type = typename event_consumed_bytes_arg::arg_type;

    using base = execution_kernel<numa_kernel<kernel_params>,
                                  send_buf_size_arg,
                                  send_buf_arg<processing_type>,
                                  recv_buf_arg<processing_type>,
                                  tmp_recv_buf_arg<processing_type>,
                                  income_data_flag_arg,
                                  ready_to_recv_flag_arg,
                                  local_barrier_flag_arg,
                                  right_tmp_recv_buf_arg<processing_type>,
                                  right_income_data_flag_arg,
                                  right_ready_to_recv_flag_arg,
                                  event_prod_chunk_mem_arg,
                                  event_prod_bytes_arg,
                                  event_consumed_bytes_offset_arg,
                                  event_consumed_chunk_mem_arg,
                                  event_consumed_bytes_arg>;

    template <class ctx_params_t>
    void bind_data(const ctx_params_t& out_ctx_params) {
        this->template set_arg<event_prod_chunk_mem_arg>(out_ctx_params.numa_staged_memory->get());
        this->template set_arg<event_prod_bytes_arg>(
            out_ctx_params.staged_memory_size_counter->get());
        this->template set_arg<event_consumed_bytes_offset_arg>(
            out_ctx_params.producer_aggregated_memory_offset->get());
        this->template set_arg<event_consumed_chunk_mem_arg>(
            out_ctx_params.total_producers_aggregated_memory->get());
        this->template set_arg<event_consumed_bytes_arg>(
            out_ctx_params.total_producers_aggregated_size_counter->get());
    }
};

template <class kernel_params>
struct ipc_kernel : public base_ipc_kernel<ipc_kernel<kernel_params>,
                                           stub_arg<main_kernel_args::args_start_index>,
                                           stub_arg<main_kernel_args::args_start_index + 1>,
                                           stub_arg<main_kernel_args::args_start_index + 2>,
                                           tmp_recv_buf_arg<typename kernel_params::native_type>,
                                           income_data_flag_arg,
                                           ready_to_recv_flag_arg,
                                           stub_arg<main_kernel_args::args_start_index + 6>,
                                           stub_arg<main_kernel_args::args_start_index + 7>,
                                           stub_arg<main_kernel_args::args_start_index + 8>,
                                           stub_arg<main_kernel_args::args_start_index + 9>> {
    using param_t = kernel_params;
    using processing_type = typename kernel_params::native_type;

    using common_entry_buf_size_arg = send_buf_size_arg;
    using common_entry_buf_arg = send_buf_arg<processing_type>;

    static constexpr const char* specific_name() {
        return "ring_allreduce_ipc";
    }

    using base = base_ipc_kernel<ipc_kernel<kernel_params>,
                                 stub_arg<main_kernel_args::args_start_index>,
                                 stub_arg<main_kernel_args::args_start_index + 1>,
                                 stub_arg<main_kernel_args::args_start_index + 2>,
                                 tmp_recv_buf_arg<processing_type>,
                                 income_data_flag_arg,
                                 ready_to_recv_flag_arg,
                                 stub_arg<main_kernel_args::args_start_index + 6>,
                                 stub_arg<main_kernel_args::args_start_index + 7>,
                                 stub_arg<main_kernel_args::args_start_index + 8>,
                                 stub_arg<main_kernel_args::args_start_index + 9>>;

    template <class ipc_handles_t>
    void bind_data(const ipc_handles_t& ipc_handles) {
        auto tmp_recv_buf = reinterpret_cast<typename tmp_recv_buf_arg<processing_type>::arg_type>(
            ipc_handles.at(0).get().pointer);
        this->template set_arg<tmp_recv_buf_arg<processing_type>>(tmp_recv_buf);

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
        : public execution_kernel<
              scale_out_cpu_gw_kernel<kernel_params>,
              send_buf_size_arg,
              send_buf_arg<typename kernel_params::native_type>,
              recv_buf_arg<typename kernel_params::native_type>,
              tmp_recv_buf_arg<typename kernel_params::native_type>,
              income_data_flag_arg,
              ready_to_recv_flag_arg,
              local_barrier_flag_arg,
              right_tmp_recv_buf_arg<typename kernel_params::native_type>,
              right_income_data_flag_arg,
              right_ready_to_recv_flag_arg,

              // scaleout-specific args
              permanent_arg<main_kernel_args::args_start_index + 10,
                            typename kernel_params::native_type*>,
              permanent_arg<main_kernel_args::args_start_index + 11, uint64_t*>,
              permanent_arg<main_kernel_args::args_start_index + 12, uint64_t*>,
              permanent_arg<main_kernel_args::args_start_index + 13,
                            typename kernel_params::native_type*>,
              permanent_arg<main_kernel_args::args_start_index + 14, uint64_t*>> {
    using param_t = kernel_params;
    using processing_type = typename param_t::native_type;

    static constexpr const char* specific_name() {
        return "allreduce_execution_scale_out_cpu_gw";
    }

    using common_entry_buf_size_arg = send_buf_size_arg;
    using common_entry_buf_arg = send_buf_arg<processing_type>;

    // event data
    using event_prod_chunk_mem_arg =
        permanent_arg<main_kernel_args::args_start_index + 10, processing_type*>;
    using event_prod_chunk_mem_arg_type = typename event_prod_chunk_mem_arg::arg_type;

    using event_prod_bytes_arg = permanent_arg<main_kernel_args::args_start_index + 11, uint64_t*>;
    using event_prod_bytes_arg_type = typename event_prod_bytes_arg::arg_type;

    using event_consumed_bytes_offset_arg =
        permanent_arg<main_kernel_args::args_start_index + 12, uint64_t*>;
    using event_consumed_bytes_offset_arg_type = typename event_consumed_bytes_offset_arg::arg_type;

    using event_consumed_chunk_mem_arg =
        permanent_arg<main_kernel_args::args_start_index + 13, processing_type*>;
    using event_consumed_chunk_mem_arg_type = typename event_consumed_chunk_mem_arg::arg_type;

    using event_consumed_bytes_arg =
        permanent_arg<main_kernel_args::args_start_index + 14, uint64_t*>;
    using event_consumed_bytes_arg_type = typename event_consumed_bytes_arg::arg_type;

    using base = execution_kernel<scale_out_cpu_gw_kernel<kernel_params>,
                                  send_buf_size_arg,
                                  send_buf_arg<processing_type>,
                                  recv_buf_arg<processing_type>,
                                  tmp_recv_buf_arg<processing_type>,
                                  income_data_flag_arg,
                                  ready_to_recv_flag_arg,
                                  local_barrier_flag_arg,
                                  right_tmp_recv_buf_arg<processing_type>,
                                  right_income_data_flag_arg,
                                  right_ready_to_recv_flag_arg,
                                  event_prod_chunk_mem_arg,
                                  event_prod_bytes_arg,
                                  event_consumed_bytes_offset_arg,
                                  event_consumed_chunk_mem_arg,
                                  event_consumed_bytes_arg>;

    template <class ctx_params_t>
    void bind_data(const ctx_params_t& out_ctx_params) {
        this->template set_arg<event_prod_chunk_mem_arg>(out_ctx_params.numa_staged_memory->get());
        this->template set_arg<event_prod_bytes_arg>(
            out_ctx_params.staged_memory_size_counter->get());
        this->template set_arg<event_consumed_bytes_offset_arg>(
            out_ctx_params.producer_aggregated_memory_offset->get());
        this->template set_arg<event_consumed_chunk_mem_arg>(
            out_ctx_params.total_producers_aggregated_memory->get());
        this->template set_arg<event_consumed_bytes_arg>(
            out_ctx_params.total_producers_aggregated_size_counter->get());
    }
};

} // namespace allreduce
} // namespace ring
} // namespace native
