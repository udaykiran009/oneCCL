#define ALGO_SELECTION_ENV "CCL_BCAST"
#define BCAST_VALUE_COEFF  128

#include "base_impl.hpp"

template <typename T>
class bcast_test : public base_test<T> {
public:
    int check(test_operation<T>& op) {
        for (size_t buf_idx = 0; buf_idx < op.buffer_count; buf_idx++) {
            for (size_t elem_idx = 0; elem_idx < op.elem_count;
                 elem_idx += op.get_check_step(elem_idx)) {
                T expected = static_cast<T>(elem_idx % BCAST_VALUE_COEFF);
                if (base_test<T>::check_error(op, expected, buf_idx, elem_idx))
                    return TEST_FAILURE;
            }
        }
        return TEST_SUCCESS;
    }

    void fill_recv_buffers(test_operation<T>& op) {
        if (op.comm_rank != ROOT_RANK)
            return;

        for (size_t buf_idx = 0; buf_idx < op.buffer_count; buf_idx++) {
            for (size_t elem_idx = 0; elem_idx < op.elem_count; elem_idx++) {
                op.recv_bufs[buf_idx][elem_idx] = elem_idx % BCAST_VALUE_COEFF;
            }
        }
    }

    void run_derived(test_operation<T>& op) {
        auto attr = ccl::create_operation_attr<ccl::broadcast_attr>();
        for (auto buf_idx : op.buf_indexes) {
            op.prepare_attr(attr, buf_idx);
            op.events.push_back(ccl::broadcast(op.get_recv_buf(buf_idx),
                                               op.elem_count,
                                               op.datatype,
                                               ROOT_RANK,
                                               global_data::instance().comms[0],
                                               attr));
        }
    }
};

RUN_METHOD_DEFINITION(bcast_test);
TEST_CASES_DEFINITION(bcast_test);
MAIN_FUNCTION();
