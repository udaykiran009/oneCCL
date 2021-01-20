#define COLL_NAME "CCL_ALLTOALL"

#include "base_impl.hpp"

template <typename T>
class alltoall_test : public base_test<T> {
public:
    int check(typed_test_param<T>& param) {
        for (size_t buf_idx = 0; buf_idx < param.buffer_count; buf_idx++) {
            for (int rank = 0; rank < param.comm_size; rank++) {
                for (size_t elem_idx = 0; elem_idx < param.elem_count; elem_idx++) {
                    T expected = static_cast<T>(rank + buf_idx);
                    size_t global_elem_idx = (param.elem_count * rank) + elem_idx;
                    if (base_test<T>::check_error(param, expected, buf_idx, global_elem_idx))
                        return TEST_FAILURE;
                }
            }
        }
        return TEST_SUCCESS;
    }

    void fill_send_buffers(typed_test_param<T>& param) {
        for (size_t buf_idx = 0; buf_idx < param.buffer_count; buf_idx++) {
            for (size_t elem_idx = 0; elem_idx < param.comm_size * param.elem_count; elem_idx++) {
                param.send_buf[buf_idx][elem_idx] = param.comm_rank + buf_idx;
            }
        }
    }

    size_t get_recv_buf_size(typed_test_param<T>& param) {
        return param.elem_count * param.comm_size;
    }

    void run_derived(typed_test_param<T>& param) {
        void* send_buf;
        void* recv_buf;

        const ccl_test_conf& test_conf = param.get_conf();
        auto attr = ccl::create_operation_attr<ccl::alltoall_attr>();
        ccl::datatype datatype = get_ccl_lib_datatype(test_conf);

        for (size_t buf_idx = 0; buf_idx < param.buffer_count; buf_idx++) {
            size_t new_idx = param.buf_indexes[buf_idx];
            param.prepare_coll_attr(attr, param.buf_indexes[buf_idx]);

            send_buf = param.get_send_buf(new_idx);
            recv_buf = param.get_recv_buf(new_idx);

            param.reqs[buf_idx] =
                ccl::alltoall((test_conf.place_type == PT_IN) ? recv_buf : send_buf,
                              recv_buf,
                              param.elem_count,
                              datatype,
                              GlobalData::instance().comms[0],
                              attr);
        }
    }
};

RUN_METHOD_DEFINITION(alltoall_test);
TEST_CASES_DEFINITION(alltoall_test);
MAIN_FUNCTION();
