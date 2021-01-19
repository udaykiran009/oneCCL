#pragma once
#include "common/comm/l0/modules/kernel_functions.hpp"

namespace native {
template <class kernel_params>
struct ring_reduce_scatter_kernel
        : public execution_kernel<
              ring_reduce_scatter_kernel<kernel_params>,
              arg<main_kernel_args::args_start_index, size_t>, // recv_count
              arg<main_kernel_args::args_start_index + 1,
                  typename kernel_params::native_type*>, // send_buf
              arg<main_kernel_args::args_start_index + 2,
                  typename kernel_params::native_type*>, // recv_buf
              external_arg<main_kernel_args::args_start_index + 3,
                           typename kernel_params::native_type*>, // tmp_buf
              external_arg<main_kernel_args::args_start_index + 4, int*>, // left_wrote_to_me_flag
              external_arg<main_kernel_args::args_start_index + 5, int*>, // i_ready_to_receive_flag
              arg<main_kernel_args::args_start_index + 6, int*>, // local_barrier_flag
              thread_exchangable_arg<main_kernel_args::args_start_index + 7,
                                     typename kernel_params::native_type*>, // right_output_buffer
              thread_exchangable_arg<main_kernel_args::args_start_index + 8,
                                     typename kernel_params::native_type*>, // right_temp_buffer
              thread_exchangable_arg<main_kernel_args::args_start_index + 9,
                                     int*>, // i_send_to_right_flag
              thread_exchangable_arg<main_kernel_args::args_start_index + 10,
                                     int*>> { // right_ready_to_recv_flag
    using param_t = kernel_params;
    using processing_type = typename kernel_params::native_type;

    static constexpr const char* specific_name() {
        return "reduce_scatter_execution";
    }

    //own
    using send_buf_size_arg = arg<main_kernel_args::args_start_index, size_t>;
    using common_entry_buf_size_arg = send_buf_size_arg;
    using send_buf_size_arg_type = typename send_buf_size_arg::arg_type;

    using send_buf_arg = arg<main_kernel_args::args_start_index + 1, processing_type*>;
    using common_entry_buf_arg = send_buf_arg;
    using send_buf_arg_type = typename send_buf_arg::arg_type;

    using recv_buf_arg = arg<main_kernel_args::args_start_index + 2, processing_type*>;
    using recv_buf_arg_type = typename recv_buf_arg::arg_type;

    using tmp_recv_buf_arg = external_arg<main_kernel_args::args_start_index + 3, processing_type*>;
    using tmp_recv_buf_arg_type = typename tmp_recv_buf_arg::arg_type;

    using income_data_flag_arg = external_arg<main_kernel_args::args_start_index + 4, int*>;
    using income_data_flag_arg_type = typename income_data_flag_arg::arg_type;

    using ready_to_recv_flag_arg = external_arg<main_kernel_args::args_start_index + 5, int*>;
    using ready_to_recv_flag_arg_type = typename ready_to_recv_flag_arg::arg_type;

    using local_barrier_flag_arg = arg<main_kernel_args::args_start_index + 6, int*>;
    using local_barrier_flag_arg_type = typename local_barrier_flag_arg::arg_type;

    using right_output_buf_arg =
        thread_exchangable_arg<main_kernel_args::args_start_index + 7, processing_type*>;
    using right_output_buf_arg_type = typename right_output_buf_arg::arg_type;

    //right
    using right_tmp_recv_buf_arg =
        thread_exchangable_arg<main_kernel_args::args_start_index + 8, processing_type*>;
    using right_tmp_recv_buf_arg_type = typename right_tmp_recv_buf_arg::arg_type;

    /*  using right_recv_buf_arg =                  thread_safe_arg<main_kernel_args::args_start_index + 8, void *>;
    using right_recv_buf_arg_type =             typename right_recv_buf_arg::arg_type;*/

    using right_income_data_flag_arg =
        thread_exchangable_arg<main_kernel_args::args_start_index + 9, int*>;
    using right_income_data_flag_arg_type = typename right_income_data_flag_arg::arg_type;

    using right_ready_to_recv_flag_arg =
        thread_exchangable_arg<main_kernel_args::args_start_index + 10, int*>;
    using right_ready_to_recv_flag_arg_type = typename right_ready_to_recv_flag_arg::arg_type;

    using base = execution_kernel<ring_reduce_scatter_kernel<kernel_params>,
                                  send_buf_size_arg,
                                  send_buf_arg,
                                  recv_buf_arg,
                                  tmp_recv_buf_arg,
                                  income_data_flag_arg,
                                  ready_to_recv_flag_arg,
                                  local_barrier_flag_arg,
                                  right_output_buf_arg,
                                  right_tmp_recv_buf_arg,
                                  right_income_data_flag_arg,
                                  right_ready_to_recv_flag_arg>;
};

template <class kernel_params>
struct ring_reduce_scatter_numa_kernel
        : public execution_kernel<
              ring_reduce_scatter_numa_kernel<kernel_params>,
              arg<main_kernel_args::args_start_index, size_t>,
              arg<main_kernel_args::args_start_index + 1, typename kernel_params::native_type*>,
              arg<main_kernel_args::args_start_index + 2, typename kernel_params::native_type*>,
              thread_safe_arg<main_kernel_args::args_start_index + 3,
                              typename kernel_params::native_type*>,
              thread_safe_arg<main_kernel_args::args_start_index + 4, int*>,
              thread_safe_arg<main_kernel_args::args_start_index + 5, int*>,
              arg<main_kernel_args::args_start_index + 6, int*>,
              thread_safe_arg<main_kernel_args::args_start_index + 7,
                              typename kernel_params::native_type*>,
              thread_safe_arg<main_kernel_args::args_start_index + 8, int*>,
              thread_safe_arg<main_kernel_args::args_start_index + 9, int*>,
              arg<main_kernel_args::args_start_index + 10, size_t>,
              thread_safe_arg<main_kernel_args::args_start_index + 11,
                              typename kernel_params::native_type*>> {
    using param_t = kernel_params;
    using processing_type = typename kernel_params::native_type;

    static constexpr const char* specific_name() {
        return "reduce_scatter_execution_numa";
    }

    //own
    using send_buf_size_arg = arg<main_kernel_args::args_start_index, size_t>;
    using send_buf_size_arg_type = typename send_buf_size_arg::arg_type;

    using send_buf_arg = arg<main_kernel_args::args_start_index + 1, processing_type*>;
    using send_buf_arg_type = typename send_buf_arg::arg_type;

    using recv_buf_arg = arg<main_kernel_args::args_start_index + 2, processing_type*>;
    using recv_buf_arg_type = typename recv_buf_arg::arg_type;

    using tmp_recv_buf_arg =
        thread_safe_arg<main_kernel_args::args_start_index + 3, processing_type*>;
    using tmp_recv_buf_arg_type = typename tmp_recv_buf_arg::arg_type;

    using income_data_flag_arg = thread_safe_arg<main_kernel_args::args_start_index + 4, int*>;
    using income_data_flag_arg_type = typename income_data_flag_arg::arg_type;

    using ready_to_recv_flag_arg = thread_safe_arg<main_kernel_args::args_start_index + 5, int*>;
    using ready_to_recv_flag_arg_type = typename ready_to_recv_flag_arg::arg_type;

    using local_barrier_flag_arg = arg<main_kernel_args::args_start_index + 6, int*>;
    using local_barrier_flag_arg_type = typename local_barrier_flag_arg::arg_type;

    //right
    using right_tmp_recv_buf_arg =
        thread_safe_arg<main_kernel_args::args_start_index + 7, processing_type*>;
    using right_tmp_recv_buf_arg_type = typename right_tmp_recv_buf_arg::arg_type;

    /*  using right_recv_buf_arg =                  thread_safe_arg<main_kernel_args::args_start_index + 8, void *>;
    using right_recv_buf_arg_type =             typename right_recv_buf_arg::arg_type;
*/
    using right_income_data_flag_arg =
        thread_safe_arg<main_kernel_args::args_start_index + 8, int*>;
    using right_income_data_flag_arg_type = typename right_income_data_flag_arg::arg_type;

    using right_ready_to_recv_flag_arg =
        thread_safe_arg<main_kernel_args::args_start_index + 9, int*>;
    using right_ready_to_recv_flag_arg_type = typename right_ready_to_recv_flag_arg::arg_type;

    // event data
    using event_prod_chunk_mem_arg = thread_safe_arg<main_kernel_args::args_start_index + 10,
                                                     typename kernel_params::native_type*>;
    using event_prod_chunk_mem_arg_type = typename event_prod_chunk_mem_arg::arg_type;

    using event_prod_bytes_arg = thread_safe_arg<main_kernel_args::args_start_index + 11, int*>;
    using event_prod_bytes_arg_type = typename event_prod_bytes_arg::arg_type;

    using base = execution_kernel<ring_reduce_scatter_numa_kernel<kernel_params>,
                                  send_buf_size_arg,
                                  send_buf_arg,
                                  recv_buf_arg,
                                  tmp_recv_buf_arg,
                                  income_data_flag_arg,
                                  ready_to_recv_flag_arg,
                                  local_barrier_flag_arg,
                                  right_tmp_recv_buf_arg,
                                  right_income_data_flag_arg,
                                  right_ready_to_recv_flag_arg,
                                  event_prod_chunk_mem_arg,
                                  event_prod_bytes_arg>;
};

template <class kernel_params>
struct ring_reduce_scatter_ipc
        : public ipc_kernel<ring_reduce_scatter_ipc<kernel_params>,
                            stub_arg<main_kernel_args::args_start_index>,
                            stub_arg<main_kernel_args::args_start_index + 1>,
                            stub_arg<main_kernel_args::args_start_index + 2>,
                            thread_safe_arg<main_kernel_args::args_start_index + 3,
                                            typename kernel_params::native_type*>,
                            thread_safe_arg<main_kernel_args::args_start_index + 4, int*>,
                            thread_safe_arg<main_kernel_args::args_start_index + 5, int*>,
                            stub_arg<main_kernel_args::args_start_index + 6>,
                            stub_arg<main_kernel_args::args_start_index + 7>,
                            stub_arg<main_kernel_args::args_start_index + 8>,
                            stub_arg<main_kernel_args::args_start_index + 9>> {
    using param_t = kernel_params;
    using processing_type = typename kernel_params::native_type;

    static constexpr const char* specific_name() {
        return "ring_reduce_scatter_ipc";
    }

    using tmp_recv_buf_arg = typename ring_reduce_scatter_kernel<kernel_params>::tmp_recv_buf_arg;
    using tmp_recv_buf_arg_type = typename tmp_recv_buf_arg::arg_type;

    using income_data_flag_arg =
        typename ring_reduce_scatter_kernel<kernel_params>::income_data_flag_arg;
    using income_data_flag_arg_type = typename income_data_flag_arg::arg_type;

    using ready_to_recv_flag_arg =
        typename ring_reduce_scatter_kernel<kernel_params>::ready_to_recv_flag_arg;
    using ready_to_recv_flag_arg_type = typename ready_to_recv_flag_arg::arg_type;

    using base = execution_kernel<ring_reduce_scatter_ipc<kernel_params>,
                                  stub_arg<main_kernel_args::args_start_index>,
                                  stub_arg<main_kernel_args::args_start_index + 1>,
                                  stub_arg<main_kernel_args::args_start_index + 2>,
                                  tmp_recv_buf_arg,
                                  income_data_flag_arg,
                                  ready_to_recv_flag_arg,
                                  stub_arg<main_kernel_args::args_start_index + 6>,
                                  stub_arg<main_kernel_args::args_start_index + 7>,
                                  stub_arg<main_kernel_args::args_start_index + 8>,
                                  stub_arg<main_kernel_args::args_start_index + 9>>;
};
} // namespace native
