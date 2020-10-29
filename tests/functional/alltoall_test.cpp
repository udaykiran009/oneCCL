#define TEST_CCL_ALLTOALL

#define COLL_NAME "CCL_ALLTOALL"

#include "base_impl.hpp"

template <typename T>
class alltoall_test : public base_test<T> {
public:
    int check(typed_test_param<T>& param) {
        for (size_t buf_idx = 0; buf_idx < param.buffer_count; buf_idx++) {
            for (size_t proc_idx = 0; proc_idx < param.process_count; proc_idx++) {
                for (size_t elem_idx = 0; elem_idx < param.elem_count; elem_idx++) {
                    T expected = static_cast<T>(proc_idx);
                    size_t size = (param.elem_count * proc_idx) + elem_idx;
                    if (base_test<T>::check_error(param, expected, buf_idx, size))
                        return TEST_FAILURE;
                }
            }
        }
        return TEST_SUCCESS;
    }

    void fill_buffers(typed_test_param<T>& param) {
        for (size_t buf_idx = 0; buf_idx < param.buffer_count; buf_idx++) {
            for (size_t proc_idx = 0; proc_idx < param.process_count * param.elem_count;
                 proc_idx++) {
                param.send_buf[buf_idx][proc_idx] = param.process_idx;
                if (param.test_conf.place_type == PT_OOP) {
                    param.recv_buf[buf_idx][proc_idx] = static_cast<T>(SOME_VALUE);
                    if (param.test_conf.datatype == DT_BFLOAT16) {
                        param.recv_buf_bf16[buf_idx][proc_idx] = static_cast<short>(SOME_VALUE);
                    }
                }
            }
        }

        if (param.test_conf.place_type != PT_OOP) {
            param.recv_buf = param.send_buf;
        }
    }

    size_t get_recv_buf_size(typed_test_param<T>& param) {
        return param.elem_count * param.process_count;
    }

    void run_derived(typed_test_param<T>& param) {
        void* send_buf;
        void* recv_buf;
        size_t count = param.elem_count;

        const ccl_test_conf& test_conf = param.get_conf();

        auto attr = ccl::create_operation_attr<ccl::alltoall_attr>();

        ccl::datatype datatype = get_ccl_lib_datatype(test_conf);

        for (size_t buf_idx = 0; buf_idx < param.buffer_count; buf_idx++) {
            size_t new_idx = param.buf_indexes[buf_idx];
            param.prepare_coll_attr(attr, param.buf_indexes[buf_idx]);

            send_buf = param.get_send_buf(new_idx);
            recv_buf = param.get_recv_buf(new_idx);

            param.reqs[buf_idx] = ccl::alltoall(
                (test_conf.place_type == PT_IN) ? recv_buf : send_buf,
                recv_buf,
                count,
                datatype,
                GlobalData::instance().comms[0],
                attr);
        }
    }
};

RUN_METHOD_DEFINITION(alltoall_test);
TEST_CASES_DEFINITION(alltoall_test);
MAIN_FUNCTION();
