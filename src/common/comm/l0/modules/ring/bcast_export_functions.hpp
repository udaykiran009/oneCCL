#pragma once
#include "common/comm/l0/modules/kernel_functions.hpp"

namespace native {
template <class native_type>
struct ring_bcast_kernel
        : public execution_kernel<
              ring_bcast_kernel<native_type>,
              arg<main_kernel_args::args_start_index, size_t>,
              thread_exchangable_arg<main_kernel_args::args_start_index + 1, native_type*>,
              external_arg<main_kernel_args::args_start_index + 2, int*>,
              external_arg<main_kernel_args::args_start_index + 3, int*>,
              arg<main_kernel_args::args_start_index + 4, int*>,
              thread_exchangable_arg<main_kernel_args::args_start_index + 5, native_type*>,
              thread_exchangable_arg<main_kernel_args::args_start_index + 6, int*>,
              thread_exchangable_arg<main_kernel_args::args_start_index + 7, int*>,
              arg<main_kernel_args::args_start_index + 8, size_t>> {
    using processing_type = native_type;

    static constexpr const char* specific_name() {
        return "bcast_execution";
    }

    //own
    using buf_size_arg = arg<main_kernel_args::args_start_index, size_t>;
    using common_entry_buf_size_arg = buf_size_arg;
    using buf_size_arg_type = typename buf_size_arg::arg_type;

    using buf_arg = arg<main_kernel_args::args_start_index + 1, processing_type*>;
    using common_entry_buf_arg = buf_arg;
    using buf_arg_type = typename buf_arg::arg_type;

    using income_data_flag_arg = external_arg<main_kernel_args::args_start_index + 2, int*>;
    using income_data_flag_arg_type = typename income_data_flag_arg::arg_type;

    using ready_to_recv_flag_arg = external_arg<main_kernel_args::args_start_index + 3, int*>;
    using ready_to_recv_flag_arg_type = typename ready_to_recv_flag_arg::arg_type;

    using local_barrier_flag_arg = arg<main_kernel_args::args_start_index + 4, int*>;
    using local_barrier_flag_arg_type = typename local_barrier_flag_arg::arg_type;

    //right
    using right_buf_arg =
        thread_exchangable_arg<main_kernel_args::args_start_index + 5, processing_type*>;
    using right_buf_arg_type = typename right_buf_arg::arg_type;

    using right_income_data_flag_arg =
        thread_exchangable_arg<main_kernel_args::args_start_index + 6, int*>;
    using right_income_data_flag_arg_type = typename right_income_data_flag_arg::arg_type;

    using right_ready_to_recv_flag_arg =
        thread_exchangable_arg<main_kernel_args::args_start_index + 7, int*>;
    using right_ready_to_recv_flag_arg_type = typename right_ready_to_recv_flag_arg::arg_type;

    using root_arg = arg<main_kernel_args::args_start_index + 8, size_t>;
    using root_arg_type = typename root_arg::arg_type;

    using base = execution_kernel<ring_bcast_kernel<native_type>,
                                  buf_size_arg,
                                  buf_arg,
                                  income_data_flag_arg,
                                  ready_to_recv_flag_arg,
                                  local_barrier_flag_arg,
                                  right_buf_arg,
                                  right_income_data_flag_arg,
                                  right_ready_to_recv_flag_arg,
                                  root_arg>;
};

template <class native_type>
struct ring_bcast_numa_kernel
        : public execution_kernel<
              ring_bcast_numa_kernel<native_type>,
              arg<main_kernel_args::args_start_index, size_t>,
              thread_safe_arg<main_kernel_args::args_start_index + 1, native_type*>,
              thread_safe_arg<main_kernel_args::args_start_index + 2, int*>,
              thread_safe_arg<main_kernel_args::args_start_index + 3, int*>,
              arg<main_kernel_args::args_start_index + 4, int*>,
              thread_safe_arg<main_kernel_args::args_start_index + 5, native_type*>,
              thread_safe_arg<main_kernel_args::args_start_index + 6, int*>,
              thread_safe_arg<main_kernel_args::args_start_index + 7, int*>,
              arg<main_kernel_args::args_start_index + 8, size_t>,

              thread_safe_arg<main_kernel_args::args_start_index + 9, native_type*>,
              thread_safe_arg<main_kernel_args::args_start_index + 10, int*>> {
    using processing_type = native_type;

    static constexpr const char* specific_name() {
        return "bcast_execution_numa";
    }

    //own
    using buf_size_arg = arg<main_kernel_args::args_start_index, size_t>;
    using buf_size_arg_type = typename buf_size_arg::arg_type;

    using buf_arg = arg<main_kernel_args::args_start_index + 1, processing_type*>;
    using buf_arg_type = typename buf_arg::arg_type;

    using income_data_flag_arg = thread_safe_arg<main_kernel_args::args_start_index + 2, int*>;
    using income_data_flag_arg_type = typename income_data_flag_arg::arg_type;

    using ready_to_recv_flag_arg = thread_safe_arg<main_kernel_args::args_start_index + 3, int*>;
    using ready_to_recv_flag_arg_type = typename ready_to_recv_flag_arg::arg_type;

    using local_barrier_flag_arg = arg<main_kernel_args::args_start_index + 4, int*>;
    using local_barrier_flag_arg_type = typename local_barrier_flag_arg::arg_type;

    //right
    using right_buf_arg = thread_safe_arg<main_kernel_args::args_start_index + 5, processing_type*>;
    using right_buf_arg_type = typename right_buf_arg::arg_type;

    using right_income_data_flag_arg =
        thread_safe_arg<main_kernel_args::args_start_index + 6, int*>;
    using right_income_data_flag_arg_type = typename right_income_data_flag_arg::arg_type;

    using right_ready_to_recv_flag_arg =
        thread_safe_arg<main_kernel_args::args_start_index + 7, int*>;
    using right_ready_to_recv_flag_arg_type = typename right_ready_to_recv_flag_arg::arg_type;

    using root_arg = arg<main_kernel_args::args_start_index + 8, size_t>;
    using root_arg_type = typename root_arg::arg_type;

    // event data
    using event_prod_chunk_mem_arg =
        thread_safe_arg<main_kernel_args::args_start_index + 9, native_type*>;
    using event_prod_chunk_mem_arg_type = typename event_prod_chunk_mem_arg::arg_type;

    using event_prod_bytes_arg = thread_safe_arg<main_kernel_args::args_start_index + 10, int*>;
    using event_prod_bytes_arg_type = typename event_prod_bytes_arg::arg_type;

    using base = execution_kernel<ring_bcast_numa_kernel<native_type>,
                                  buf_size_arg,
                                  buf_arg,
                                  income_data_flag_arg,
                                  ready_to_recv_flag_arg,
                                  local_barrier_flag_arg,
                                  right_buf_arg,
                                  right_income_data_flag_arg,
                                  right_ready_to_recv_flag_arg,
                                  root_arg,
                                  event_prod_chunk_mem_arg,
                                  event_prod_bytes_arg>;
};

template <class native_type>
struct ring_bcast_ipc
        : public ipc_kernel<ring_bcast_ipc<native_type>,
                            stub_arg<main_kernel_args::args_start_index>,
                            thread_safe_arg<main_kernel_args::args_start_index + 1, native_type*>,
                            thread_safe_arg<main_kernel_args::args_start_index + 2, int*>,
                            thread_safe_arg<main_kernel_args::args_start_index + 3, int*>,
                            stub_arg<main_kernel_args::args_start_index + 4>,
                            stub_arg<main_kernel_args::args_start_index + 5>,
                            stub_arg<main_kernel_args::args_start_index + 6>,
                            stub_arg<main_kernel_args::args_start_index + 7>> {
    using processing_type = native_type;

    static constexpr const char* specific_name() {
        return "ring_bcast_ipc";
    }

    using recv_buf_arg = typename ring_bcast_kernel<native_type>::buf_arg;
    using recv_buf_arg_type = typename recv_buf_arg::arg_type;

    using income_data_flag_arg = typename ring_bcast_kernel<native_type>::income_data_flag_arg;
    using income_data_flag_arg_type = typename income_data_flag_arg::arg_type;

    using ready_to_recv_flag_arg = typename ring_bcast_kernel<native_type>::ready_to_recv_flag_arg;
    using ready_to_recv_flag_arg_type = typename ready_to_recv_flag_arg::arg_type;

    using base = execution_kernel<ring_bcast_ipc<native_type>,
                                  stub_arg<main_kernel_args::args_start_index>,
                                  recv_buf_arg,
                                  income_data_flag_arg,
                                  ready_to_recv_flag_arg,
                                  stub_arg<main_kernel_args::args_start_index + 4>,
                                  stub_arg<main_kernel_args::args_start_index + 5>,
                                  stub_arg<main_kernel_args::args_start_index + 6>,
                                  stub_arg<main_kernel_args::args_start_index + 7>>;
};
} // namespace native
