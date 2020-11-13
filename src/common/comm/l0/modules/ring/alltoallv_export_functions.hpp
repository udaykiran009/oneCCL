#pragma once
#include "common/comm/l0/modules/kernel_functions.hpp"

namespace native {
template <class native_type>
struct ring_alltoallv_kernel
        : public execution_kernel<
              ring_alltoallv_kernel<native_type>,
              arg<main_kernel_args::args_start_index, size_t*>, // send_elem_counts
              arg<main_kernel_args::args_start_index + 1, size_t*>, // send_elem_offsets
              arg<main_kernel_args::args_start_index + 2, size_t*>, // recv_elem_counts_buf
              arg<main_kernel_args::args_start_index + 3, size_t*>, // recv_elem_offsets_buf
              arg<main_kernel_args::args_start_index + 4, native_type*>, // send_buf
              arg<main_kernel_args::args_start_index + 5, native_type*>, // recv_buf
              thread_exchangable_arg<main_kernel_args::args_start_index + 6,
                                     native_type*>, // tmp_buffer
              thread_exchangable_arg<main_kernel_args::args_start_index + 7,
                                     native_type*>, // right_temp_buffer
              thread_exchangable_arg<main_kernel_args::args_start_index + 8,
                                     int*>, // left_wrote_to_me_flag
              thread_exchangable_arg<main_kernel_args::args_start_index + 9,
                                     int*>, // i_ready_to_receive_flag
              thread_exchangable_arg<main_kernel_args::args_start_index + 10,
                                     int*>, // proxy_size_flag
              thread_exchangable_arg<main_kernel_args::args_start_index + 11,
                                     int*>, // i_send_to_right_flag
              thread_exchangable_arg<main_kernel_args::args_start_index + 12,
                                     int*>, // right_ready_to_recv_flag
              thread_exchangable_arg<main_kernel_args::args_start_index + 13,
                                     int*>> // right_proxy_size_flag
{
    using processing_type = native_type;

    static constexpr const char* specific_name() {
        return "alltoallv_execution";
    }

    // send_elem_counts
    using send_buf_size_arg = arg<main_kernel_args::args_start_index, size_t*>;
    using common_entry_buf_size_arg = send_buf_size_arg;
    using send_buf_size_arg_type = typename send_buf_size_arg::arg_type;

    // send_elem_offsets
    using send_elem_offsets_buf_arg = arg<main_kernel_args::args_start_index + 1, size_t*>;
    using send_elem_offsets_buf_arg_type = typename send_elem_offsets_buf_arg::arg_type;

    // recv_elem_counts_buf
    using recv_elem_counts_buf_arg = arg<main_kernel_args::args_start_index + 2, size_t*>;
    using recv_elem_counts_buf_arg_type = typename recv_elem_counts_buf_arg::arg_type;

    // recv_elem_offsets_buf
    using recv_elem_offsets_buf_arg = arg<main_kernel_args::args_start_index + 3, size_t*>;
    using recv_elem_offsets_buf_arg_type = typename recv_elem_offsets_buf_arg::arg_type;

    // send_buf
    using send_buf_arg = arg<main_kernel_args::args_start_index + 4, processing_type*>;
    using common_entry_buf_arg = send_buf_arg;
    using send_buf_arg_type = typename send_buf_arg::arg_type;

    // recv_buf
    using recv_buf_arg = arg<main_kernel_args::args_start_index + 5, processing_type*>;
    using recv_buf_arg_type = typename recv_buf_arg::arg_type;

    // tmp_buffer
    using tmp_recv_buf_arg =
        thread_exchangable_arg<main_kernel_args::args_start_index + 6, processing_type*>;
    using tmp_recv_buf_arg_type = typename tmp_recv_buf_arg::arg_type;

    // right_temp_buffer
    using right_tmp_recv_buf_arg =
        thread_exchangable_arg<main_kernel_args::args_start_index + 7, processing_type*>;
    using right_tmp_recv_buf_arg_type = typename right_tmp_recv_buf_arg::arg_type;

    // left_wrote_to_me_flag
    using income_data_flag_arg =
        thread_exchangable_arg<main_kernel_args::args_start_index + 8, int*>;
    using income_data_flag_arg_type = typename income_data_flag_arg::arg_type;

    // i_ready_to_receive_flag
    using ready_to_recv_flag_arg =
        thread_exchangable_arg<main_kernel_args::args_start_index + 9, int*>;
    using ready_to_recv_flag_arg_type = typename ready_to_recv_flag_arg::arg_type;

    // proxy_size_flag
    using proxy_size_flag_arg =
        thread_exchangable_arg<main_kernel_args::args_start_index + 10, int*>;
    using proxy_size_flag_arg_type = typename proxy_size_flag_arg::arg_type;

    // i_send_to_right_flag
    using right_income_data_flag_arg =
        thread_exchangable_arg<main_kernel_args::args_start_index + 11, int*>;
    using right_income_data_flag_arg_type = typename right_income_data_flag_arg::arg_type;

    // right_ready_to_recv_flag
    using right_ready_to_recv_flag_arg =
        thread_exchangable_arg<main_kernel_args::args_start_index + 12, int*>;
    using right_ready_to_recv_flag_arg_type = typename right_ready_to_recv_flag_arg::arg_type;

    // right_proxy_size_flag
    using right_proxy_size_flag_arg =
        thread_exchangable_arg<main_kernel_args::args_start_index + 13, int*>;
    using right_proxy_size_flag_type = typename right_proxy_size_flag_arg::arg_type;

    using base = execution_kernel<ring_alltoallv_kernel<native_type>,
                                  send_buf_size_arg, // 0 send_elem_counts
                                  send_elem_offsets_buf_arg, // 1 send_elem_offsets
                                  recv_elem_counts_buf_arg, // 2 recv_elem_counts
                                  recv_elem_offsets_buf_arg, // 3 recv_elem_offsets
                                  send_buf_arg, // 4 send_buf_arg
                                  recv_buf_arg, // 5 recv_buf_arg
                                  tmp_recv_buf_arg, // 6 tmp_buffer
                                  right_tmp_recv_buf_arg, // 7 right_temp_buffer
                                  income_data_flag_arg, // 8 left_wrote_to_me_flag
                                  ready_to_recv_flag_arg, // 9 i_ready_to_receive_flag
                                  proxy_size_flag_arg, // 10 proxy_size_flag_arg
                                  right_income_data_flag_arg, // 11 i_send_to_right_flag
                                  right_ready_to_recv_flag_arg, // 12 right_ready_to_recv_flag
                                  right_proxy_size_flag_arg>; // 13 right_proxy_size_flag
};

// IMPORTANT: the params order is default, see *altoallv*.cl for that
template <class native_type>
struct ring_alltoallv_numa_kernel
        : public execution_kernel<
              ring_alltoallv_numa_kernel<native_type>,
              arg<main_kernel_args::args_start_index, size_t*>, // send_elem_counts
              arg<main_kernel_args::args_start_index + 1, size_t*>, // send_elem_offsets
              arg<main_kernel_args::args_start_index + 2, size_t*>, // recv_elem_counts_buf
              arg<main_kernel_args::args_start_index + 3, size_t*>, // recv_elem_offsets_buf
              arg<main_kernel_args::args_start_index + 4, native_type*>, // send_buf
              arg<main_kernel_args::args_start_index + 5, native_type*>, // recv_buf
              thread_safe_arg<main_kernel_args::args_start_index + 6, native_type*>, // tmp_buffer
              thread_safe_arg<main_kernel_args::args_start_index + 7,
                              native_type*>, // right_temp_buffer
              thread_safe_arg<main_kernel_args::args_start_index + 8,
                              int*>, // left_wrote_to_me_flag
              thread_safe_arg<main_kernel_args::args_start_index + 9,
                              int*>, // i_ready_to_receive_flag
              thread_safe_arg<main_kernel_args::args_start_index + 10, int*>, // proxy_size_flag
              thread_safe_arg<main_kernel_args::args_start_index + 11,
                              int*>, // i_send_to_right_flag
              thread_safe_arg<main_kernel_args::args_start_index + 12,
                              int*>, // right_ready_to_recv_flag
              thread_safe_arg<main_kernel_args::args_start_index + 13,
                              int*>> // right_proxy_size_flag
{
    using processing_type = native_type;

    static constexpr const char* specific_name() {
        return "alltoallv_execution_numa";
    }

    // send_elem_counts
    using send_buf_size_arg = arg<main_kernel_args::args_start_index, size_t*>;
    using common_entry_buf_size_arg = send_buf_size_arg;
    using send_buf_size_arg_type = typename send_buf_size_arg::arg_type;

    // send_elem_offsets
    using send_elem_offsets_buf_arg = arg<main_kernel_args::args_start_index + 1, size_t*>;
    using send_elem_offsets_buf_arg_type = typename send_elem_offsets_buf_arg::arg_type;

    // recv_elem_counts_buf
    using recv_elem_counts_buf_arg = arg<main_kernel_args::args_start_index + 2, size_t*>;
    using recv_elem_counts_buf_arg_type = typename recv_elem_counts_buf_arg::arg_type;

    // recv_elem_offsets_buf
    using recv_elem_offsets_buf_arg = arg<main_kernel_args::args_start_index + 3, size_t*>;
    using recv_elem_offsets_buf_arg_type = typename recv_elem_offsets_buf_arg::arg_type;

    // send_buf
    using send_buf_arg = arg<main_kernel_args::args_start_index + 4, processing_type*>;
    using common_entry_buf_arg = send_buf_arg;
    using send_buf_arg_type = typename send_buf_arg::arg_type;

    // recv_buf
    using recv_buf_arg = arg<main_kernel_args::args_start_index + 5, processing_type*>;
    using recv_buf_arg_type = typename recv_buf_arg::arg_type;

    // tmp_buffer
    using tmp_recv_buf_arg =
        thread_safe_arg<main_kernel_args::args_start_index + 6, processing_type*>;
    using tmp_recv_buf_arg_type = typename tmp_recv_buf_arg::arg_type;

    // right_temp_buffer
    using right_tmp_recv_buf_arg =
        thread_safe_arg<main_kernel_args::args_start_index + 7, processing_type*>;
    using right_tmp_recv_buf_arg_type = typename right_tmp_recv_buf_arg::arg_type;

    // left_wrote_to_me_flag
    using income_data_flag_arg = thread_safe_arg<main_kernel_args::args_start_index + 8, int*>;
    using income_data_flag_arg_type = typename income_data_flag_arg::arg_type;

    // i_ready_to_receive_flag
    using ready_to_recv_flag_arg = thread_safe_arg<main_kernel_args::args_start_index + 9, int*>;
    using ready_to_recv_flag_arg_type = typename ready_to_recv_flag_arg::arg_type;

    // proxy_size_flag
    using proxy_size_flag_arg = thread_safe_arg<main_kernel_args::args_start_index + 10, int*>;
    using proxy_size_flag_arg_type = typename proxy_size_flag_arg::arg_type;

    // i_send_to_right_flag
    using right_income_data_flag_arg =
        thread_safe_arg<main_kernel_args::args_start_index + 11, int*>;
    using right_income_data_flag_arg_type = typename right_income_data_flag_arg::arg_type;

    // right_ready_to_recv_flag
    using right_ready_to_recv_flag_arg =
        thread_safe_arg<main_kernel_args::args_start_index + 12, int*>;
    using right_ready_to_recv_flag_arg_type = typename right_ready_to_recv_flag_arg::arg_type;

    // right_proxy_size_flag
    using right_proxy_size_flag_arg =
        thread_safe_arg<main_kernel_args::args_start_index + 13, int*>;
    using right_proxy_size_flag_type = typename right_proxy_size_flag_arg::arg_type;

    using base = execution_kernel<ring_alltoallv_numa_kernel<native_type>,
                                  send_buf_size_arg, // 0 send_elem_counts
                                  send_elem_offsets_buf_arg, // 1 send_elem_offsets
                                  recv_elem_counts_buf_arg, // 2 recv_elem_counts
                                  recv_elem_offsets_buf_arg, // 3 recv_elem_offsets
                                  send_buf_arg, // 4 send_buf_arg
                                  recv_buf_arg, // 5 recv_buf_arg
                                  tmp_recv_buf_arg, // 6 tmp_buffer
                                  right_tmp_recv_buf_arg, // 7 right_temp_buffer
                                  income_data_flag_arg, // 8 left_wrote_to_me_flag
                                  ready_to_recv_flag_arg, // 9 i_ready_to_receive_flag
                                  proxy_size_flag_arg, // 10 proxy_size_flag_arg
                                  right_income_data_flag_arg, // 11 i_send_to_right_flag
                                  right_ready_to_recv_flag_arg, // 12 right_ready_to_recv_flag
                                  right_proxy_size_flag_arg>; // 13 right_proxy_size_flag
};

template <class native_type>
struct ring_alltoallv_ipc
        : public ipc_kernel<
              ring_alltoallv_ipc<native_type>,
              arg<main_kernel_args::args_start_index, size_t*>, // send_elem_counts
              arg<main_kernel_args::args_start_index + 1, size_t*>, // send_elem_offsets
              arg<main_kernel_args::args_start_index + 2, size_t*>, // recv_elem_counts_buf
              arg<main_kernel_args::args_start_index + 3, size_t*>, // recv_elem_offsets_buf
              arg<main_kernel_args::args_start_index + 4, native_type*>, // send_buf
              arg<main_kernel_args::args_start_index + 5, native_type*>, // recv_buf
              thread_safe_arg<main_kernel_args::args_start_index + 6, native_type*>, // tmp_buffer
              thread_safe_arg<main_kernel_args::args_start_index + 7,
                              native_type*>, // right_temp_buffer
              thread_safe_arg<main_kernel_args::args_start_index + 8,
                              int*>, // left_wrote_to_me_flag
              thread_safe_arg<main_kernel_args::args_start_index + 9,
                              int*>, // i_ready_to_receive_flag
              thread_safe_arg<main_kernel_args::args_start_index + 10, int*>, // proxy_size_flag
              thread_safe_arg<main_kernel_args::args_start_index + 11,
                              int*>, // i_send_to_right_flag
              thread_safe_arg<main_kernel_args::args_start_index + 12,
                              int*>, // right_ready_to_recv_flag
              thread_safe_arg<main_kernel_args::args_start_index + 13,
                              int*>> // right_proxy_size_flag
{
    using processing_type = native_type;

    static constexpr const char* specific_name() {
        return "ring_alltoallv_ipc";
    }

    // send_elem_counts
    using send_buf_size_arg = arg<main_kernel_args::args_start_index, size_t*>;
    using common_entry_buf_size_arg = send_buf_size_arg;
    using send_buf_size_arg_type = typename send_buf_size_arg::arg_type;

    // send_elem_offsets
    using send_elem_offsets_buf_arg = arg<main_kernel_args::args_start_index + 1, size_t*>;
    using send_elem_offsets_buf_arg_type = typename send_elem_offsets_buf_arg::arg_type;

    // recv_elem_counts_buf
    using recv_elem_counts_buf_arg = arg<main_kernel_args::args_start_index + 2, size_t*>;
    using recv_elem_counts_buf_arg_type = typename recv_elem_counts_buf_arg::arg_type;

    // recv_elem_offsets_buf
    using recv_elem_offsets_buf_arg = arg<main_kernel_args::args_start_index + 3, size_t*>;
    using recv_elem_offsets_buf_arg_type = typename recv_elem_offsets_buf_arg::arg_type;

    // send_buf
    using send_buf_arg = arg<main_kernel_args::args_start_index + 4, processing_type*>;
    using common_entry_buf_arg = send_buf_arg;
    using send_buf_arg_type = typename send_buf_arg::arg_type;

    // recv_buf
    using recv_buf_arg = arg<main_kernel_args::args_start_index + 5, processing_type*>;
    using recv_buf_arg_type = typename recv_buf_arg::arg_type;

    // tmp_buffer
    using tmp_recv_buf_arg =
        thread_safe_arg<main_kernel_args::args_start_index + 6, processing_type*>;
    using tmp_recv_buf_arg_type = typename tmp_recv_buf_arg::arg_type;

    // right_temp_buffer
    using right_tmp_recv_buf_arg =
        thread_safe_arg<main_kernel_args::args_start_index + 7, processing_type*>;
    using right_tmp_recv_buf_arg_type = typename right_tmp_recv_buf_arg::arg_type;

    // left_wrote_to_me_flag
    using income_data_flag_arg = thread_safe_arg<main_kernel_args::args_start_index + 8, int*>;
    using income_data_flag_arg_type = typename income_data_flag_arg::arg_type;

    // i_ready_to_receive_flag
    using ready_to_recv_flag_arg = thread_safe_arg<main_kernel_args::args_start_index + 9, int*>;
    using ready_to_recv_flag_arg_type = typename ready_to_recv_flag_arg::arg_type;

    // proxy_size_flag
    using proxy_size_flag_arg = thread_safe_arg<main_kernel_args::args_start_index + 10, int*>;
    using proxy_size_flag_arg_type = typename proxy_size_flag_arg::arg_type;

    // i_send_to_right_flag
    using right_income_data_flag_arg =
        thread_safe_arg<main_kernel_args::args_start_index + 11, int*>;
    using right_income_data_flag_arg_type = typename right_income_data_flag_arg::arg_type;

    // right_ready_to_recv_flag
    using right_ready_to_recv_flag_arg =
        thread_safe_arg<main_kernel_args::args_start_index + 12, int*>;
    using right_ready_to_recv_flag_arg_type = typename right_ready_to_recv_flag_arg::arg_type;

    // right_proxy_size_flag
    using right_proxy_size_flag_arg =
        thread_safe_arg<main_kernel_args::args_start_index + 13, int*>;
    using right_proxy_size_flag_type = typename right_proxy_size_flag_arg::arg_type;

    using base = execution_kernel<ring_alltoallv_ipc<native_type>,
                                  send_buf_size_arg, // 0 send_elem_counts
                                  send_elem_offsets_buf_arg, // 1 send_elem_offsets
                                  recv_elem_counts_buf_arg, // 2 recv_elem_counts
                                  recv_elem_offsets_buf_arg, // 3 recv_elem_offsets
                                  send_buf_arg, // 4 send_buf_arg
                                  recv_buf_arg, // 5 recv_buf_arg
                                  tmp_recv_buf_arg, // 6 tmp_buffer
                                  right_tmp_recv_buf_arg, // 7 right_temp_buffer
                                  income_data_flag_arg, // 8 left_wrote_to_me_flag
                                  ready_to_recv_flag_arg, // 9 i_ready_to_receive_flag
                                  proxy_size_flag_arg, // 10 proxy_size_flag_arg
                                  right_income_data_flag_arg, // 11 i_send_to_right_flag
                                  right_ready_to_recv_flag_arg, // 12 right_ready_to_recv_flag
                                  right_proxy_size_flag_arg>; // 13 right_proxy_size_flag
};
} // namespace native
