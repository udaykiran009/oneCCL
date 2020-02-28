#define TEST_CCL_REDUCE
#define Collective_Name "CCL_ALLREDUCE"

#include "base_impl.hpp"
#include "base_bfp16.hpp"

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
                    if (this->check_error(param, expected, buf_idx, elem_idx))
                        return TEST_FAILURE;
                }

                if (param.test_conf.reduction_type == RT_MAX)
                {
                    T expected = get_expected_max<T>(elem_idx, buf_idx, param.process_count);
                    if (this->check_error(param, expected, buf_idx, elem_idx))
                        return TEST_FAILURE;
                }
                if (param.test_conf.reduction_type == RT_MIN)
                {
                    T expected = get_expected_min<T>(elem_idx, buf_idx, param.process_count);
                    if (this->check_error(param, expected, buf_idx, elem_idx))
                        return TEST_FAILURE;
                }
                if (param.test_conf.reduction_type == RT_PROD)
                {
                    T expected = 1;
                    for (size_t k = 0; k < param.process_count; k++)
                    {
                        expected *= elem_idx + buf_idx + k;
                    }
                    if (this->check_error(param, expected, buf_idx, elem_idx))
                        return TEST_FAILURE;
                }
            }
        }
        return TEST_SUCCESS;
    }

    void run_derived(typed_test_param<T>& param)
    {
        void* send_buf;
        void* recv_buf;
        size_t count = param.elem_count;
        size_t size = param.elem_count;
        const ccl_test_conf& test_conf = param.get_conf();
        ccl::reduction reduction = (ccl::reduction) test_conf.reduction_type;
        ccl::coll_attr* attr = &param.coll_attr;
        ccl::stream_t& stream = param.get_stream();
        ccl::data_type data_type = static_cast<ccl::data_type>(test_conf.data_type);
            for (size_t buf_idx = 0; buf_idx < param.buffer_count; buf_idx++)
            {
                size_t new_idx = param.buf_indexes[buf_idx];
                param.prepare_coll_attr(param.buf_indexes[buf_idx]);
                if (test_conf.data_type == DT_BFP16)
                {
                    send_buf = static_cast<void*>(param.send_buf_bfp16[new_idx].data());
                    recv_buf = static_cast<void*>(param.recv_buf_bfp16[new_idx].data());
                    prepare_bfp16_buffers(param, send_buf, recv_buf, new_idx, size);
                }
                else
                {
                    send_buf = static_cast<void*>(param.send_buf[new_idx].data());
                    recv_buf = static_cast<void*>(param.recv_buf[new_idx].data());
                }
                param.reqs[buf_idx] =
                    param.global_comm->allreduce((test_conf.place_type == PT_IN) ? recv_buf : send_buf, 
                                            recv_buf, count, data_type, reduction, attr, stream);
            }
            param.complete();
            if (test_conf.data_type == DT_BFP16)
            {
                copy_to_recv_buf(param, size);
            }
    }
};

RUN_METHOD_DEFINITION(allreduce_test);
TEST_CASES_DEFINITION(allreduce_test);
MAIN_FUNCTION();
