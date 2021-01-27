#define ALGO_SELECTION_ENV "CCL_REDUCE"

#include "base_impl.hpp"

template <typename T>
class reduce_test : public base_test<T> {
public:
    int check(typed_test_param<T>& param) {
        if (param.comm_rank != ROOT_RANK)
            return TEST_SUCCESS;

        for (size_t buf_idx = 0; buf_idx < param.buffer_count; buf_idx++) {
            for (size_t elem_idx = 0; elem_idx < param.elem_count; elem_idx++) {
                T expected = base_test<T>::calculate_reduce_value(param, buf_idx, elem_idx);
                if (base_test<T>::check_error(param, expected, buf_idx, elem_idx))
                    return TEST_FAILURE;
            }
        }
        return TEST_SUCCESS;
    }

    void run_derived(typed_test_param<T>& param) {
        void* send_buf;
        void* recv_buf;

        const ccl_test_conf& test_conf = param.get_conf();
        auto attr = ccl::create_operation_attr<ccl::reduce_attr>();
        ccl::reduction reduction = get_ccl_lib_reduction(test_conf);
        ccl::datatype datatype = get_ccl_lib_datatype(test_conf);

        for (size_t buf_idx = 0; buf_idx < param.buffer_count; buf_idx++) {
            size_t new_idx = param.buf_indexes[buf_idx];
            param.prepare_coll_attr(attr, param.buf_indexes[buf_idx]);

            send_buf = param.get_send_buf(new_idx);
            recv_buf = param.get_recv_buf(new_idx);

            param.reqs[buf_idx] = ccl::reduce((test_conf.place_type == PT_IN) ? recv_buf : send_buf,
                                              recv_buf,
                                              param.elem_count,
                                              datatype,
                                              reduction,
                                              ROOT_RANK,
                                              GlobalData::instance().comms[0],
                                              attr);
        }
    }
};

RUN_METHOD_DEFINITION(reduce_test);
TEST_CASES_DEFINITION(reduce_test);
MAIN_FUNCTION();
