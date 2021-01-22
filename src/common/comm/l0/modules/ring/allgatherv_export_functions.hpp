#pragma once
#include "common/comm/l0/modules/kernel_functions.hpp"

namespace native {
template <class kernel_params>
struct ring_allgatherv_kernel
        : public execution_kernel<
              ring_allgatherv_kernel<kernel_params>,
              arg<main_kernel_args::args_start_index, size_t>, // elems_count
              arg<main_kernel_args::args_start_index + 1, size_t*>, // recv_elem_counts_buf
              arg<main_kernel_args::args_start_index + 2, size_t*>, // recv_elem_offsets_buf
              arg<main_kernel_args::args_start_index + 3,
                  typename kernel_params::native_type*>, // send_buf
              thread_exchangable_arg<main_kernel_args::args_start_index + 4,
                                     typename kernel_params::native_type*>, // recv_buf
              arg<main_kernel_args::args_start_index + 5,
                  typename kernel_params::native_type*>, // right_output_buffer
              external_arg<main_kernel_args::args_start_index + 6,
                           int*>, // left_wrote_to_me_flag
              external_arg<main_kernel_args::args_start_index + 7,
                           int*>, // i_ready_to_receive_flag
              thread_exchangable_arg<main_kernel_args::args_start_index + 8,
                                     int*>, // i_send_to_right_flag
              thread_exchangable_arg<main_kernel_args::args_start_index + 9,
                                     int*>> // right_ready_to_recv_flag
{
    using processing_type = typename kernel_params::native_type;

    static constexpr const char* specific_name() {
        return "allgatherv_execution";
    }

    // elems_count
    using send_buf_size_arg = arg<main_kernel_args::args_start_index, size_t>;
    using common_entry_buf_size_arg = send_buf_size_arg;
    using send_buf_size_arg_type = typename send_buf_size_arg::arg_type;

    // recv_elem_counts_buf
    using recv_elem_counts_buf_arg = arg<main_kernel_args::args_start_index + 1, size_t*>;
    using recv_elem_counts_buf_arg_type = typename recv_elem_counts_buf_arg::arg_type;

    // recv_elem_offsets_buf
    using recv_elem_offsets_buf_arg = arg<main_kernel_args::args_start_index + 2, size_t*>;
    using recv_elem_offsets_buf_arg_type = typename recv_elem_offsets_buf_arg::arg_type;

    // send_buf
    using send_buf_arg = arg<main_kernel_args::args_start_index + 3, processing_type*>;
    using common_entry_buf_arg = send_buf_arg;
    using send_buf_arg_type = typename send_buf_arg::arg_type;

    // recv_buf
    using recv_buf_arg = arg<main_kernel_args::args_start_index + 4, processing_type*>;
    using recv_buf_arg_type = typename recv_buf_arg::arg_type;

    // right_output_buffer
    using right_output_buf_arg =
        thread_exchangable_arg<main_kernel_args::args_start_index + 5, processing_type*>;
    using right_output_buf_arg_type = typename right_output_buf_arg::arg_type;

    // left_wrote_to_me_flag
    using income_data_flag_arg = external_arg<main_kernel_args::args_start_index + 6, int*>;
    using income_data_flag_arg_type = typename income_data_flag_arg::arg_type;

    // i_ready_to_receive_flag
    using ready_to_recv_flag_arg = external_arg<main_kernel_args::args_start_index + 7, int*>;
    using ready_to_recv_flag_arg_type = typename ready_to_recv_flag_arg::arg_type;

    // i_send_to_right_flag
    using right_income_data_flag_arg =
        thread_exchangable_arg<main_kernel_args::args_start_index + 8, int*>;
    using right_income_data_flag_arg_type = typename right_income_data_flag_arg::arg_type;

    // right_ready_to_recv_flag
    using right_ready_to_recv_flag_arg =
        thread_exchangable_arg<main_kernel_args::args_start_index + 9, int*>;
    using right_ready_to_recv_flag_arg_type = typename right_ready_to_recv_flag_arg::arg_type;

    using base = execution_kernel<ring_allgatherv_kernel<kernel_params>,
                                  send_buf_size_arg,
                                  recv_elem_counts_buf_arg,
                                  recv_elem_offsets_buf_arg,
                                  send_buf_arg,
                                  recv_buf_arg,
                                  right_output_buf_arg,
                                  income_data_flag_arg,
                                  ready_to_recv_flag_arg,
                                  right_income_data_flag_arg,
                                  right_ready_to_recv_flag_arg>;
};

// IMPORTANT: the params order is default, see *algatherv*.cl for that
template <class kernel_params>
struct ring_allgatherv_numa_kernel
        : public execution_kernel<
              ring_allgatherv_numa_kernel<kernel_params>,
              arg<main_kernel_args::args_start_index, size_t>, // elems_count
              arg<main_kernel_args::args_start_index + 1, size_t*>, // recv_elem_counts_buf
              arg<main_kernel_args::args_start_index + 2, size_t*>, // recv_elem_offsets_buf
              arg<main_kernel_args::args_start_index + 3,
                  typename kernel_params::native_type*>, // send_buf
              arg<main_kernel_args::args_start_index + 4,
                  typename kernel_params::native_type*>, // recv_buf
              thread_safe_arg<main_kernel_args::args_start_index + 5,
                              typename kernel_params::native_type*>, // right_output_buffer
              thread_safe_arg<main_kernel_args::args_start_index + 6,
                              int*>, // left_wrote_to_me_flag
              thread_safe_arg<main_kernel_args::args_start_index + 7,
                              int*>, // i_ready_to_receive_flag
              thread_safe_arg<main_kernel_args::args_start_index + 8, int*>, // i_send_to_right_flag
              thread_safe_arg<main_kernel_args::args_start_index + 9,
                              int*>> // right_ready_to_recv_flag>
{
    using processing_type = typename kernel_params::native_type;

    static constexpr const char* specific_name() {
        return "allgatherv_execution_numa";
    }

    // elems_count
    using send_buf_size_arg = arg<main_kernel_args::args_start_index, size_t>;
    using common_entry_buf_size_arg = send_buf_size_arg;
    using send_buf_size_arg_type = typename send_buf_size_arg::arg_type;

    // recv_elem_counts_buf
    using recv_elem_counts_buf_arg = arg<main_kernel_args::args_start_index + 1, size_t*>;
    using recv_elem_counts_buf_arg_type = typename recv_elem_counts_buf_arg::arg_type;

    // recv_elem_offsets_buf
    using recv_elem_offsets_buf_arg = arg<main_kernel_args::args_start_index + 2, size_t*>;
    using recv_elem_offsets_buf_arg_type = typename recv_elem_offsets_buf_arg::arg_type;

    // send_buf
    using send_buf_arg = arg<main_kernel_args::args_start_index + 3, processing_type*>;
    using common_entry_buf_arg = send_buf_arg;
    using send_buf_arg_type = typename send_buf_arg::arg_type;

    // recv_buf
    using recv_buf_arg = arg<main_kernel_args::args_start_index + 4, processing_type*>;
    using recv_buf_arg_type = typename recv_buf_arg::arg_type;

    // right_output_buffer
    using right_output_buf_arg =
        thread_safe_arg<main_kernel_args::args_start_index + 5, processing_type*>;
    using right_output_buf_arg_type = typename right_output_buf_arg::arg_type;

    // left_wrote_to_me_flag
    using income_data_flag_arg = thread_safe_arg<main_kernel_args::args_start_index + 6, int*>;
    using income_data_flag_arg_type = typename income_data_flag_arg::arg_type;

    // i_ready_to_receive_flag
    using ready_to_recv_flag_arg = thread_safe_arg<main_kernel_args::args_start_index + 7, int*>;
    using ready_to_recv_flag_arg_type = typename ready_to_recv_flag_arg::arg_type;

    // i_send_to_right_flag
    using right_income_data_flag_arg =
        thread_safe_arg<main_kernel_args::args_start_index + 8, int*>;
    using right_income_data_flag_arg_type = typename right_income_data_flag_arg::arg_type;

    // right_ready_to_recv_flag
    using right_ready_to_recv_flag_arg =
        thread_safe_arg<main_kernel_args::args_start_index + 9, int*>;
    using right_ready_to_recv_flag_arg_type = typename right_ready_to_recv_flag_arg::arg_type;

    using base = execution_kernel<ring_allgatherv_numa_kernel<kernel_params>,
                                  send_buf_size_arg,
                                  recv_elem_counts_buf_arg,
                                  recv_elem_offsets_buf_arg,
                                  send_buf_arg,
                                  recv_buf_arg,
                                  right_output_buf_arg,
                                  income_data_flag_arg,
                                  ready_to_recv_flag_arg,
                                  right_income_data_flag_arg,
                                  right_ready_to_recv_flag_arg>;
};

template <class kernel_params>
struct ring_allgatherv_ipc
        : public ipc_kernel<
              ring_allgatherv_ipc<kernel_params>,
              arg<main_kernel_args::args_start_index, size_t>, // elems_count
              arg<main_kernel_args::args_start_index + 1, size_t*>, // recv_elem_counts_buf
              arg<main_kernel_args::args_start_index + 2, size_t*>, // recv_elem_offsets_buf
              arg<main_kernel_args::args_start_index + 3,
                  typename kernel_params::native_type*>, // send_buf
              arg<main_kernel_args::args_start_index + 4,
                  typename kernel_params::native_type*>, // recv_buf
              thread_safe_arg<main_kernel_args::args_start_index + 5,
                              typename kernel_params::native_type*>, // right_output_buffer
              thread_safe_arg<main_kernel_args::args_start_index + 6,
                              int*>, // left_wrote_to_me_flag
              thread_safe_arg<main_kernel_args::args_start_index + 7,
                              int*>, // i_ready_to_receive_flag
              thread_safe_arg<main_kernel_args::args_start_index + 8, int*>, // i_send_to_right_flag
              thread_safe_arg<main_kernel_args::args_start_index + 9,
                              int*>> // right_ready_to_recv_flag
{
    using processing_type = typename kernel_params::native_type;

    static constexpr const char* specific_name() {
        return "ring_allgatherv_ipc";
    }

    // elems_count
    using send_buf_size_arg = arg<main_kernel_args::args_start_index, size_t>;
    using common_entry_buf_size_arg = send_buf_size_arg;
    using send_buf_size_arg_type = typename send_buf_size_arg::arg_type;

    // recv_elem_counts_buf
    using recv_elem_counts_buf_arg = arg<main_kernel_args::args_start_index + 1, size_t*>;
    using recv_elem_counts_buf_arg_type = typename recv_elem_counts_buf_arg::arg_type;

    // recv_elem_offsets_buf
    using recv_elem_offsets_buf_arg = arg<main_kernel_args::args_start_index + 2, size_t*>;
    using recv_elem_offsets_buf_arg_type = typename recv_elem_offsets_buf_arg::arg_type;

    // send_buf
    using send_buf_arg = arg<main_kernel_args::args_start_index + 3, processing_type*>;
    using common_entry_buf_arg = send_buf_arg;
    using send_buf_arg_type = typename send_buf_arg::arg_type;

    // recv_buf
    using recv_buf_arg = arg<main_kernel_args::args_start_index + 4, processing_type*>;
    using recv_buf_arg_type = typename recv_buf_arg::arg_type;

    // right_output_buffer
    using right_output_buf_arg =
        thread_safe_arg<main_kernel_args::args_start_index + 5, processing_type*>;
    using right_output_buf_arg_type = typename right_output_buf_arg::arg_type;

    // left_wrote_to_me_flag
    using income_data_flag_arg = thread_safe_arg<main_kernel_args::args_start_index + 6, int*>;
    using income_data_flag_arg_type = typename income_data_flag_arg::arg_type;

    // i_ready_to_receive_flag
    using ready_to_recv_flag_arg = thread_safe_arg<main_kernel_args::args_start_index + 7, int*>;
    using ready_to_recv_flag_arg_type = typename ready_to_recv_flag_arg::arg_type;

    // i_send_to_right_flag
    using right_income_data_flag_arg =
        thread_safe_arg<main_kernel_args::args_start_index + 8, int*>;
    using right_income_data_flag_arg_type = typename right_income_data_flag_arg::arg_type;

    // right_ready_to_recv_flag
    using right_ready_to_recv_flag_arg =
        thread_safe_arg<main_kernel_args::args_start_index + 9, int*>;
    using right_ready_to_recv_flag_arg_type = typename right_ready_to_recv_flag_arg::arg_type;

    using base = execution_kernel<ring_allgatherv_ipc<kernel_params>,
                                  send_buf_size_arg,
                                  recv_elem_counts_buf_arg,
                                  recv_elem_offsets_buf_arg,
                                  send_buf_arg,
                                  recv_buf_arg,
                                  right_output_buf_arg,
                                  income_data_flag_arg,
                                  ready_to_recv_flag_arg,
                                  right_income_data_flag_arg,
                                  right_ready_to_recv_flag_arg>;
};

template <class kernel_params>
struct ring_allgatherv_scale_out_cpu_gw_kernel
        : public execution_kernel<
              ring_allgatherv_scale_out_cpu_gw_kernel<kernel_params>,
              arg<main_kernel_args::args_start_index, size_t>, // elems_count
              arg<main_kernel_args::args_start_index + 1, size_t*>, // recv_elem_counts_buf
              arg<main_kernel_args::args_start_index + 2, size_t*>, // recv_elem_offsets_buf
              arg<main_kernel_args::args_start_index + 3,
                  typename kernel_params::native_type*>, // send_buf
              arg<main_kernel_args::args_start_index + 4,
                  typename kernel_params::native_type*>, // recv_buf
              thread_safe_arg<main_kernel_args::args_start_index + 5,
                              typename kernel_params::native_type*>, // right_output_buffer
              thread_safe_arg<main_kernel_args::args_start_index + 6,
                              int*>, // left_wrote_to_me_flag
              thread_safe_arg<main_kernel_args::args_start_index + 7,
                              int*>, // i_ready_to_receive_flag
              thread_safe_arg<main_kernel_args::args_start_index + 8, int*>, // i_send_to_right_flag
              thread_safe_arg<main_kernel_args::args_start_index + 9,
                              int*>> // right_ready_to_recv_flag>
{
    using param_t = kernel_params;
    using processing_type = typename param_t::native_type;

    static constexpr const char* specific_name() {
        return "allgatherv_execution_scale_out_cpu_gw";
    }

    // elems_count
    using send_buf_size_arg = arg<main_kernel_args::args_start_index, size_t>;
    using common_entry_buf_size_arg = send_buf_size_arg;
    using send_buf_size_arg_type = typename send_buf_size_arg::arg_type;

    // recv_elem_counts_buf
    using recv_elem_counts_buf_arg = arg<main_kernel_args::args_start_index + 1, size_t*>;
    using recv_elem_counts_buf_arg_type = typename recv_elem_counts_buf_arg::arg_type;

    // recv_elem_offsets_buf
    using recv_elem_offsets_buf_arg = arg<main_kernel_args::args_start_index + 2, size_t*>;
    using recv_elem_offsets_buf_arg_type = typename recv_elem_offsets_buf_arg::arg_type;

    // send_buf
    using send_buf_arg = arg<main_kernel_args::args_start_index + 3, processing_type*>;
    using common_entry_buf_arg = send_buf_arg;
    using send_buf_arg_type = typename send_buf_arg::arg_type;

    // recv_buf
    using recv_buf_arg = arg<main_kernel_args::args_start_index + 4, processing_type*>;
    using recv_buf_arg_type = typename recv_buf_arg::arg_type;

    // right_output_buffer
    using right_output_buf_arg =
        thread_safe_arg<main_kernel_args::args_start_index + 5, processing_type*>;
    using right_output_buf_arg_type = typename right_output_buf_arg::arg_type;

    // left_wrote_to_me_flag
    using income_data_flag_arg = thread_safe_arg<main_kernel_args::args_start_index + 6, int*>;
    using income_data_flag_arg_type = typename income_data_flag_arg::arg_type;

    // i_ready_to_receive_flag
    using ready_to_recv_flag_arg = thread_safe_arg<main_kernel_args::args_start_index + 7, int*>;
    using ready_to_recv_flag_arg_type = typename ready_to_recv_flag_arg::arg_type;

    // i_send_to_right_flag
    using right_income_data_flag_arg =
        thread_safe_arg<main_kernel_args::args_start_index + 8, int*>;
    using right_income_data_flag_arg_type = typename right_income_data_flag_arg::arg_type;

    // right_ready_to_recv_flag
    using right_ready_to_recv_flag_arg =
        thread_safe_arg<main_kernel_args::args_start_index + 9, int*>;
    using right_ready_to_recv_flag_arg_type = typename right_ready_to_recv_flag_arg::arg_type;

    using base = execution_kernel<ring_allgatherv_scale_out_cpu_gw_kernel<param_t>,
                                  send_buf_size_arg,
                                  recv_elem_counts_buf_arg,
                                  recv_elem_offsets_buf_arg,
                                  send_buf_arg,
                                  recv_buf_arg,
                                  right_output_buf_arg,
                                  income_data_flag_arg,
                                  ready_to_recv_flag_arg,
                                  right_income_data_flag_arg,
                                  right_ready_to_recv_flag_arg>;
};

} // namespace native
