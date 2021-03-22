#define ALGO_SELECTION_ENV "CCL_REDUCE_SCATTER"

#include "test_impl.hpp"

template <typename T>
class reduce_scatter_test : public base_test<T> {
public:
    int check(test_operation<T>& op) {
        int my_rank = transport_data::instance().get_rank();
        for (size_t buf_idx = 0; buf_idx < op.buffer_count; buf_idx++) {
            for (size_t elem_idx = 0; elem_idx < op.elem_count;
                 elem_idx += op.get_check_step(elem_idx)) {
                size_t real_elem_idx = my_rank * op.elem_count + elem_idx;
                T expected = base_test<T>::calculate_reduce_value(op, buf_idx, real_elem_idx);
                if (base_test<T>::check_error(op, expected, buf_idx, elem_idx))
                    return TEST_FAILURE;
            }
        }
        return TEST_SUCCESS;
    }

    void run_derived(test_operation<T>& op) {
        void* send_buf;
        void* recv_buf;

        auto param = op.get_param();
        auto attr = ccl::create_operation_attr<ccl::reduce_scatter_attr>();

        for (auto buf_idx : op.buf_indexes) {
            op.prepare_attr(attr, buf_idx);
            send_buf = op.get_send_buf(buf_idx);
            recv_buf = op.get_recv_buf(buf_idx);

            op.events.push_back(
                ccl::reduce_scatter((param.place_type == PLACE_IN) ? recv_buf : send_buf,
                                    recv_buf,
                                    op.elem_count,
                                    op.datatype,
                                    op.reduction,
                                    transport_data::instance().get_comm(),
                                    transport_data::instance().get_stream(),
                                    attr));
        }
    }
};

RUN_METHOD_DEFINITION(reduce_scatter_test);
TEST_CASES_DEFINITION(reduce_scatter_test);
MAIN_FUNCTION();
