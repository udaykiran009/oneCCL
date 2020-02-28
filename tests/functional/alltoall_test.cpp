#define Collective_Name "CCL_ALLTOALL"

#include "base_bfp16.hpp"
#include "base_impl.hpp"

template <typename T> class alltoall_test : public base_test <T>
{
public:

     int check(typed_test_param<T>& param)
     {
        for (size_t buf_idx = 0; buf_idx < param.buffer_count; buf_idx++)
        {
            for (size_t proc_idx = 0; proc_idx < param.process_count; proc_idx++)
            {
                for (size_t elem_idx = 0; elem_idx < param.elem_count; elem_idx++)
                {
                    T expected = static_cast<T>(proc_idx);
                    size_t size = (param.elem_count * proc_idx) + elem_idx;
                    if (this->check_error(param, expected, buf_idx, size))
                        return TEST_FAILURE;
                }
            }
        }
        return TEST_SUCCESS;
    }

    void fill_buffers(typed_test_param<T>& param)
    {
        for (size_t buf_idx = 0; buf_idx < param.buffer_count; buf_idx++)
        {
            for (size_t proc_idx = 0; proc_idx < param.process_count * param.elem_count; proc_idx++)
            {
                param.send_buf[buf_idx][proc_idx] = param.process_idx;
                if (param.test_conf.place_type == PT_OOP)
                {
                    param.recv_buf[buf_idx][proc_idx] = static_cast<T>(SOME_VALUE);
                    if (param.test_conf.data_type == DT_BFP16)
                    {
                        param.recv_buf_bfp16[buf_idx][proc_idx] = static_cast<short>(SOME_VALUE);
                    }
                }
            }
        }
        if (param.test_conf.place_type != PT_OOP)
        {
            param.recv_buf = param.send_buf;
        }
    }

    void run_derived(typed_test_param<T>& param)
    {
        void* send_buf;
        void* recv_buf;
        size_t count = param.elem_count;
        size_t size =  param.buffer_count * param.elem_count;
        const ccl_test_conf& test_conf = param.get_conf();
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
                param.global_comm->alltoall((test_conf.place_type == PT_IN) ? recv_buf : send_buf, 
                                             recv_buf, count, data_type, attr, stream);    
        }
        param.complete();
        if (test_conf.data_type == DT_BFP16)
        {
            copy_to_recv_buf(param, size);
        }
    }
};

RUN_METHOD_DEFINITION(alltoall_test);
TEST_CASES_DEFINITION(alltoall_test);
MAIN_FUNCTION();
