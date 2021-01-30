#define ALGO_SELECTION_ENV "CCL_BCAST"
#define BCAST_VALUE_COEFF  128

#include "base_impl.hpp"

template <typename T>
class bcast_test : public base_test<T> {
public:
    int check(typed_test_param<T>& param) {
        for (size_t buf_idx = 0; buf_idx < param.buffer_count; buf_idx++) {
            for (size_t elem_idx = 0; elem_idx < param.elem_count; elem_idx++) {
                T expected = static_cast<T>(elem_idx % BCAST_VALUE_COEFF);
                if (base_test<T>::check_error(param, expected, buf_idx, elem_idx))
                    return TEST_FAILURE;
            }
        }
        return TEST_SUCCESS;
    }

    void fill_recv_buffers(typed_test_param<T>& param) {
        if (param.comm_rank != ROOT_RANK)
            return;

        for (size_t buf_idx = 0; buf_idx < param.buffer_count; buf_idx++) {
            for (size_t elem_idx = 0; elem_idx < param.elem_count; elem_idx++) {
                param.recv_buf[buf_idx][elem_idx] = elem_idx % BCAST_VALUE_COEFF;
            }
        }
    }

    void run_derived(typed_test_param<T>& param) {
        const ccl_test_conf& test_conf = param.get_conf();
        auto attr = ccl::create_operation_attr<ccl::broadcast_attr>();
        ccl::datatype datatype = get_ccl_lib_datatype(test_conf);

        for (auto buf_idx : param.buf_indexes) {
            param.prepare_coll_attr(attr, buf_idx);
            param.events.push_back(ccl::broadcast(param.get_recv_buf(buf_idx),
                                                  param.elem_count,
                                                  datatype,
                                                  ROOT_RANK,
                                                  GlobalData::instance().comms[0],
                                                  attr));
        }
    }
};

RUN_METHOD_DEFINITION(bcast_test);
TEST_CASES_DEFINITION(bcast_test);
MAIN_FUNCTION();
