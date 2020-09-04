#define TEST_CCL_BCAST

#define COLL_NAME "CCL_BCAST"

#include "base_impl.hpp"

template <typename T> class bcast_test : public base_test <T>
{
public:

    int check(typed_test_param<T>& param)
    {
        for (size_t buf_idx = 0; buf_idx < param.buffer_count; buf_idx++)
        {
            for (size_t elem_idx = 0; elem_idx < param.elem_count; elem_idx++)
            {
                T expected = static_cast<T>(elem_idx);
                if (base_test<T>::check_error(param, expected, buf_idx, elem_idx))
                    return TEST_FAILURE;
            }
        }
        return TEST_SUCCESS;
    }

    void fill_buffers(typed_test_param<T>& param)
    {
        for (size_t buf_idx = 0; buf_idx < param.buffer_count; buf_idx++)
        {
            for (size_t elem_idx = 0; elem_idx < param.elem_count; elem_idx++)
            {
                if (param.process_idx == ROOT_PROCESS_IDX)
                {
                    param.recv_buf[buf_idx][elem_idx] = elem_idx;
                }
                else
                {
                    param.recv_buf[buf_idx][elem_idx] = static_cast<T>(SOME_VALUE);
                    if (param.test_conf.data_type == DT_BFP16)
                    {
                        param.recv_buf_bfp16[buf_idx][elem_idx] = static_cast<short>(SOME_VALUE);
                    }
                }
            }
            param.send_buf[buf_idx] = param.recv_buf[buf_idx];
        }
    }

    size_t get_recv_buf_size(typed_test_param<T>& param)
    {
        return param.elem_count;
    }

    void run_derived(typed_test_param<T>& param)
    {
        void* recv_buf;
        size_t count = param.elem_count;
        const ccl_test_conf& test_conf = param.get_conf();
        auto attr = ccl::environment::instance().create_op_attr<ccl::bcast_attr_t>();
        ccl::datatype data_type = static_cast<ccl::datatype>(test_conf.data_type);

        for (size_t buf_idx = 0; buf_idx < param.buffer_count; buf_idx++)
        {
            size_t new_idx = param.buf_indexes[buf_idx];
            param.prepare_coll_attr(attr, param.buf_indexes[buf_idx]);

            recv_buf = param.get_recv_buf(new_idx);

            param.reqs[buf_idx] =
                    param.global_comm.bcast(recv_buf, count, (ccl_datatype_t)data_type, ROOT_PROCESS_IDX, attr);
        }
    }
};

RUN_METHOD_DEFINITION(bcast_test);
TEST_CASES_DEFINITION(bcast_test);
MAIN_FUNCTION();
