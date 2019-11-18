#define TEST_CCL_REDUCE
#define Collective_Name "CCL_ALLREDUCE"

#include <chrono>
#include <functional>
#include <vector>

#include "base_impl.hpp"

template <typename T> class allreduce_test : public base_test <T>
{
public:
    int check(typed_test_param<T>& param)
    {
        for (size_t buf_idx = 0; buf_idx < param.buffer_count; buf_idx++)
        {
            for (size_t elem_idx = 0; elem_idx < param.elem_count; elem_idx++)
            {
                if (param.test_conf.reduction_type == RT_SUM)
                {
                    T expected =
                        ((param.process_count * (param.process_count - 1) / 2) +
                        ((elem_idx + buf_idx) * param.process_count));
                    if (param.recv_buf[buf_idx][elem_idx] != expected)
                    {
                        snprintf(allreduce_test::get_err_message(), ERR_MESSAGE_MAX_LEN, "[%zu] sent send_buf[%zu][%zu] = %f, got recv_buf[%zu][%zu] = %f, but expected = %f\n",
                                 param.process_idx, buf_idx, elem_idx, (double) param.send_buf[buf_idx][elem_idx], buf_idx,
                                 elem_idx, (double) param.recv_buf[buf_idx][elem_idx], (double) expected);
                        return TEST_FAILURE;
                    }
                }

                if (param.test_conf.reduction_type == RT_MAX)
                {
                    T expected = get_expected_max<T>(elem_idx, buf_idx, param.process_count);
                    if (param.recv_buf[buf_idx][elem_idx] != expected)
                    {
                        snprintf(allreduce_test::get_err_message(), ERR_MESSAGE_MAX_LEN, "[%zu] sent send_buf[%zu][%zu] = %f, got recv_buf[%zu][%zu] = %f, but expected = %f\n",
                                 param.process_idx, buf_idx, elem_idx, (double) param.send_buf[buf_idx][elem_idx], buf_idx,
                                 elem_idx, (double) param.recv_buf[buf_idx][elem_idx], (double) expected);
                        return TEST_FAILURE;
                    }
                }
                if (param.test_conf.reduction_type == RT_MIN)
                {
                    T expected = get_expected_min<T>(elem_idx, buf_idx, param.process_count);
                    if (param.recv_buf[buf_idx][elem_idx] != expected)
                    {
                        snprintf(allreduce_test::get_err_message(), ERR_MESSAGE_MAX_LEN, "[%zu] sent send_buf[%zu][%zu] = %f, got recv_buf[%zu][%zu] = %f, but expected = %f\n",
                                 param.process_idx, buf_idx, elem_idx, (double) param.send_buf[buf_idx][elem_idx], buf_idx,
                                 elem_idx, (double) param.recv_buf[buf_idx][elem_idx], (double) expected);
                        return TEST_FAILURE;
                    }
                }
                if (param.test_conf.reduction_type == RT_PROD)
                {
                    T expected = 1;
                    for (size_t k = 0; k < param.process_count; k++)
                    {
                        expected *= elem_idx + buf_idx + k;
                    }
                    if (param.recv_buf[buf_idx][elem_idx] != expected)
                    {
                        snprintf(allreduce_test::get_err_message(), ERR_MESSAGE_MAX_LEN, "[%zu] sent send_buf[%zu][%zu] = %f, got recv_buf[%zu][%zu] = %f, but expected = %f\n",
                                 param.process_idx, buf_idx, elem_idx, (double) param.send_buf[buf_idx][elem_idx], buf_idx,
                                 elem_idx, (double) param.recv_buf[buf_idx][elem_idx], (double) expected);
                        return TEST_FAILURE;
                    }
                }
            }
        }
        return TEST_SUCCESS;
    }

    void run_derived(typed_test_param<T>& param)
    {
        const ccl_test_conf& test_conf = param.get_conf();
        size_t count = param.elem_count;
        ccl::reduction reduction = (ccl::reduction) test_conf.reduction_type;
        ccl::coll_attr* attr = &param.coll_attr;
        ccl::stream_t& stream = param.get_stream();
        for (size_t buf_idx = 0; buf_idx < param.buffer_count; buf_idx++)
        {
            param.prepare_coll_attr(param.buf_indexes[buf_idx]);
            T* send_buf = param.send_buf[param.buf_indexes[buf_idx]].data();
            T* recv_buf = param.recv_buf[param.buf_indexes[buf_idx]].data();
            param.reqs[buf_idx] =
                param.global_comm->allreduce((test_conf.place_type == PT_IN) ? recv_buf : send_buf,
                                             recv_buf, count, reduction, attr, stream);
        }
    }
};

RUN_METHOD_DEFINITION(allreduce_test);
TEST_CASES_DEFINITION(allreduce_test);
MAIN_FUNCTION();
