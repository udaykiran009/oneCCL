#pragma once
#include "common/comm/l0/modules/kernel_functions.hpp"

namespace native
{
template<class native_type>
struct ring_allreduce_kernel : public execution_kernel<ring_allreduce_kernel<native_type>,
                                                       arg<main_kernel_args::args_start_index, size_t>,
                                                       arg<main_kernel_args::args_start_index + 1, native_type*>,
                                                       arg<main_kernel_args::args_start_index + 2, native_type*>,
                                                       thread_safe_arg<main_kernel_args::args_start_index + 3, native_type*>,
                                                       thread_safe_arg<main_kernel_args::args_start_index + 4, int*>,
                                                       thread_safe_arg<main_kernel_args::args_start_index + 5, int*>,
                                                       arg<main_kernel_args::args_start_index + 6, int*>,
                                                       thread_safe_arg<main_kernel_args::args_start_index + 7, native_type*>,
                                                       thread_safe_arg<main_kernel_args::args_start_index + 8, int*>,
                                                       thread_safe_arg<main_kernel_args::args_start_index + 9, int*>>
{
    using processing_type = native_type;

    static constexpr const char* specific_name()
    {
        return "allreduce_execution";
    }

    //own
    using send_buf_size_arg =       arg<main_kernel_args::args_start_index, size_t>;
    using send_buf_size_arg_type =  typename send_buf_size_arg::arg_type;

    using send_buf_arg =            arg<main_kernel_args::args_start_index + 1, processing_type*>;
    using send_buf_arg_type =       typename send_buf_arg::arg_type;

    using recv_buf_arg =            arg<main_kernel_args::args_start_index + 2, processing_type*>;
    using recv_buf_arg_type =       typename recv_buf_arg::arg_type;

    using tmp_recv_buf_arg =        thread_safe_arg<main_kernel_args::args_start_index + 3, processing_type*>;
    using tmp_recv_buf_arg_type =   typename tmp_recv_buf_arg::arg_type;

    using income_data_flag_arg =        thread_safe_arg<main_kernel_args::args_start_index + 4, int*>;
    using income_data_flag_arg_type =   typename income_data_flag_arg::arg_type;

    using ready_to_recv_flag_arg =        thread_safe_arg<main_kernel_args::args_start_index + 5, int*>;
    using ready_to_recv_flag_arg_type =   typename ready_to_recv_flag_arg::arg_type;

    using local_barrier_flag_arg =        arg<main_kernel_args::args_start_index + 6, int*>;
    using local_barrier_flag_arg_type =   typename local_barrier_flag_arg::arg_type;

    //right
    using right_tmp_recv_buf_arg =              thread_safe_arg<main_kernel_args::args_start_index + 7, processing_type*>;
    using right_tmp_recv_buf_arg_type =         typename right_tmp_recv_buf_arg::arg_type;

/*  using right_recv_buf_arg =                  thread_safe_arg<main_kernel_args::args_start_index + 8, void *>;
    using right_recv_buf_arg_type =             typename right_recv_buf_arg::arg_type;
*/
    using right_income_data_flag_arg =          thread_safe_arg<main_kernel_args::args_start_index + 8, int*>;
    using right_income_data_flag_arg_type =     typename right_income_data_flag_arg::arg_type;

    using right_ready_to_recv_flag_arg =        thread_safe_arg<main_kernel_args::args_start_index + 9, int*>;
    using right_ready_to_recv_flag_arg_type =   typename right_ready_to_recv_flag_arg::arg_type;

    using base = execution_kernel<ring_allreduce_kernel<native_type>,
                                  send_buf_size_arg,
                                  send_buf_arg,
                                  recv_buf_arg,
                                  tmp_recv_buf_arg,
                                  income_data_flag_arg,
                                  ready_to_recv_flag_arg,
                                  local_barrier_flag_arg,
                                  right_tmp_recv_buf_arg,
                                  right_income_data_flag_arg,
                                  right_ready_to_recv_flag_arg>;
};

template<class native_type>
struct ring_allreduce_ipc : public ipc_kernel<ring_allreduce_ipc<native_type>,
                                         stub_arg<main_kernel_args::args_start_index>,
                                         stub_arg<main_kernel_args::args_start_index + 1>,
                                         stub_arg<main_kernel_args::args_start_index + 2>,
                                         thread_safe_arg<main_kernel_args::args_start_index + 3, native_type*>,
                                         thread_safe_arg<main_kernel_args::args_start_index + 4, int*>,
                                         thread_safe_arg<main_kernel_args::args_start_index + 5, int*>,
                                         stub_arg<main_kernel_args::args_start_index + 6>,
                                         stub_arg<main_kernel_args::args_start_index + 7>,
                                         stub_arg<main_kernel_args::args_start_index + 8>,
                                         stub_arg<main_kernel_args::args_start_index + 9>>
{
    static constexpr const char* specific_name()
    {
        return "ring_allreduce_ipc";
    }

    using tmp_recv_buf_arg =              typename ring_allreduce_kernel<native_type>::tmp_recv_buf_arg;
    using tmp_recv_buf_arg_type =         typename tmp_recv_buf_arg::arg_type;

    using income_data_flag_arg =          typename ring_allreduce_kernel<native_type>::income_data_flag_arg;
    using income_data_flag_arg_type =     typename income_data_flag_arg::arg_type;

    using ready_to_recv_flag_arg =        typename ring_allreduce_kernel<native_type>::ready_to_recv_flag_arg;
    using ready_to_recv_flag_arg_type =   typename ready_to_recv_flag_arg::arg_type;

    using base = execution_kernel<ring_allreduce_ipc<native_type>,
                                  stub_arg<main_kernel_args::args_start_index >,
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
}
