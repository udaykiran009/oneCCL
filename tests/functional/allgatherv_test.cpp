#define Collective_Name "CCL_ALLGATHERV"
#define TEST_CCL_ALLGATHERV

#include <chrono>
#include <functional>
#include <vector>

#include "base_impl.hpp"
#include "base_bfp16.hpp"

template <typename T> class allgatherv_test : public base_test <T>
{
public:
    std::vector<size_t> recv_counts;
    std::vector<size_t> offsets;
    int check(typed_test_param<T>& param)
    {
        for (size_t buf_idx = 0; buf_idx < param.buffer_count; buf_idx++)
        {
            for (size_t elem_idx = 0; elem_idx < param.process_count; elem_idx++)
            {
                for (size_t recv_count_idx = 0; recv_count_idx < recv_counts[elem_idx]; recv_count_idx++)
                {
                    size_t idx = offsets[elem_idx] + recv_count_idx;
                    T expected = static_cast<T>(elem_idx + recv_count_idx);
                    if (this->check_error(param, expected, buf_idx, idx))
                    {
                        return TEST_FAILURE;
                    }
                }
            }
        }
        return TEST_SUCCESS;
    }

    void alloc_buffers(typed_test_param<T>& param)
    {
        base_test<T>::alloc_buffers(param);
        recv_counts.resize(param.process_count);
        offsets.resize(param.process_count);
        offsets[0] = 0;
        recv_counts[0] = param.elem_count;
        if (param.test_conf.place_type == PT_OOP)
        {
            for (size_t elem_idx = 0; elem_idx < param.buffer_count; elem_idx++)
            {
                /* each buffer is different size */
                param.recv_buf[elem_idx].resize(param.elem_count * param.process_count);
                if (param.test_conf.data_type == DT_BFP16)
                {
                    param.recv_buf_bfp16[elem_idx].resize(param.elem_count * param.process_count);
                }
            }
        }
    }

    void fill_buffers(typed_test_param<T>& param)
    {
        for (size_t proc_idx = 1; proc_idx < param.process_count; proc_idx++)
        {
            recv_counts[proc_idx] = (param.elem_count > proc_idx) ? param.elem_count - proc_idx : param.elem_count;
            offsets[proc_idx] = recv_counts[proc_idx - 1] + offsets[proc_idx - 1];
        }
        for (size_t buf_idx = 0; buf_idx < param.buffer_count; buf_idx++)
        {
            for (size_t elem_idx = 0; elem_idx < param.process_count; elem_idx++)
            {
                for (size_t recv_count_idx = 0; recv_count_idx < recv_counts[elem_idx]; recv_count_idx++)
                {
                    param.send_buf[buf_idx][offsets[elem_idx] + recv_count_idx] = param.process_idx + elem_idx + recv_count_idx;
                    if (param.test_conf.place_type == PT_OOP)
                    {
                        param.recv_buf[buf_idx][offsets[elem_idx] + recv_count_idx] = static_cast<T>(SOME_VALUE);
                        if (param.test_conf.data_type == DT_BFP16)
                        {
                            param.recv_buf_bfp16[buf_idx][offsets[elem_idx] + recv_count_idx] = static_cast<short>(SOME_VALUE);
                        }
                    }
                }
            }
            /* in case of in-place i-th process already has result in i-th block of send buffer */
            if (param.test_conf.place_type != PT_OOP)
            {
                param.recv_buf[buf_idx] = param.send_buf[buf_idx];
                for (size_t recv_count_idx = 0; recv_count_idx < recv_counts[param.process_idx]; recv_count_idx++)
                {
                    param.send_buf[buf_idx][offsets[param.process_idx] + recv_count_idx] = param.process_idx + recv_count_idx;
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
        size_t count = recv_counts[param.process_idx];
        size_t* recv_count = recv_counts.data();
        size_t size = param.elem_count * param.process_count;
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
                param.global_comm->allgatherv((test_conf.place_type == PT_IN) ? recv_buf : send_buf, 
                                               count, recv_buf, recv_count, data_type, attr, stream);
        }
        param.complete();
        if (test_conf.data_type == DT_BFP16)
        {
            copy_to_recv_buf(param, size);
        }
    }
};

RUN_METHOD_DEFINITION(allgatherv_test);
TEST_CASES_DEFINITION(allgatherv_test);
MAIN_FUNCTION();