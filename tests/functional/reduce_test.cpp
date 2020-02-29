#define TEST_CCL_REDUCE

#define COLL_NAME "CCL_REDUCE"

#include "base_impl.hpp"

template <typename T> class reduce_test : public base_test <T>
{
public:

    int check(typed_test_param<T>& param)
    {
        if (param.process_idx == ROOT_PROCESS_IDX)
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
                        if (base_test<T>::check_error(param, expected, buf_idx, elem_idx))
                            return TEST_FAILURE;
                    }

                    if (param.test_conf.reduction_type == RT_MAX)
                    {
                        T expected = get_expected_max<T>(elem_idx, buf_idx, param.process_count);
                        if (base_test<T>::check_error(param, expected, buf_idx, elem_idx))
                            return TEST_FAILURE;
                    }

                    if (param.test_conf.reduction_type == RT_MIN)
                    {
                        T expected = get_expected_min<T>(elem_idx, buf_idx, param.process_count);
                        if (base_test<T>::check_error(param, expected, buf_idx, elem_idx))
                            return TEST_FAILURE;
                    }

                    if (param.test_conf.reduction_type == RT_PROD)
                    {
                        T expected = 1;
                        for (size_t proc_idx = 0; proc_idx < param.process_count; proc_idx++)
                        {
                            expected *= elem_idx + buf_idx + proc_idx;
                        }
                        if (base_test<T>::check_error(param, expected, buf_idx, elem_idx))
                            return TEST_FAILURE;
                    }
                }
            }
        }
        return TEST_SUCCESS;
    }

    size_t get_recv_buf_size(typed_test_param<T>& param)
    {
        return param.elem_count;
    }

    void run_derived(typed_test_param<T>& param)
    {
        void* send_buf;
        void* recv_buf;
        size_t count = param.elem_count;
        const ccl_test_conf& test_conf = param.get_conf();
        ccl::reduction reduction = (ccl::reduction) test_conf.reduction_type;
        ccl::coll_attr* attr = &param.coll_attr;
        ccl::stream_t& stream = param.get_stream();
        ccl::data_type data_type = static_cast<ccl::data_type>(param.test_conf.data_type);

        for (size_t buf_idx = 0; buf_idx < param.buffer_count; buf_idx++)
        {
            size_t new_idx = param.buf_indexes[buf_idx];
            param.prepare_coll_attr(param.buf_indexes[buf_idx]);

            send_buf = param.get_send_buf(new_idx);
            recv_buf = param.get_recv_buf(new_idx);

            param.reqs[buf_idx] =
                param.global_comm->reduce((test_conf.place_type == PT_IN) ? recv_buf : send_buf, 
                                           recv_buf, count, data_type, reduction, ROOT_PROCESS_IDX, attr, stream);
        }
    }
};

RUN_METHOD_DEFINITION(reduce_test);
TEST_CASES_DEFINITION(reduce_test);
MAIN_FUNCTION();
