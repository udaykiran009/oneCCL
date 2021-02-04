#define ALGO_SELECTION_ENV "CCL_ALLGATHERV"

#include "base_impl.hpp"

template <typename T>
class allgatherv_test : public base_test<T> {
public:
    std::vector<size_t> recv_counts;
    std::vector<size_t> offsets;

    int check(test_operation<T>& op) {
        for (size_t buf_idx = 0; buf_idx < op.buffer_count; buf_idx++) {
            for (int rank = 0; rank < op.comm_size; rank++) {
                for (size_t elem_idx = 0; elem_idx < recv_counts[rank]; elem_idx++) {
                    size_t idx = offsets[rank] + elem_idx;
                    T expected = static_cast<T>(rank + buf_idx);
                    if (base_test<T>::check_error(op, expected, buf_idx, idx)) {
                        return TEST_FAILURE;
                    }
                }
            }
        }
        return TEST_SUCCESS;
    }

    void alloc_buffers(test_operation<T>& op) {
        recv_counts.resize(op.comm_size);
        offsets.resize(op.comm_size);
        recv_counts[0] = op.elem_count;
        offsets[0] = 0;

        for (int rank = 1; rank < op.comm_size; rank++) {
            recv_counts[rank] = ((int)op.elem_count > rank) ? op.elem_count - rank : op.elem_count;
            offsets[rank] = recv_counts[rank - 1] + offsets[rank - 1];
        }

        if (op.param.place_type == PLACE_OUT) {
            size_t total_recv_count = std::accumulate(recv_counts.begin(), recv_counts.end(), 0);
            for (size_t buf_idx = 0; buf_idx < op.buffer_count; buf_idx++) {
                op.recv_bufs[buf_idx].resize(total_recv_count);
                if (is_lp_datatype(op.param.datatype)) {
                    op.recv_bufs_lp[buf_idx].resize(total_recv_count);
                }
            }
        }
    }

    void fill_send_buffers(test_operation<T>& op) {
        for (size_t buf_idx = 0; buf_idx < op.buffer_count; buf_idx++) {
            for (size_t elem_idx = 0; elem_idx < recv_counts[op.comm_rank]; elem_idx++) {
                op.send_bufs[buf_idx][elem_idx] = op.comm_rank + buf_idx;
            }
        }
    }

    void fill_recv_buffers(test_operation<T>& op) {
        if (op.param.place_type != PLACE_IN)
            return;

        /* in case of in-place i-th rank already has result in i-th block of send buffer */
        for (size_t buf_idx = 0; buf_idx < op.buffer_count; buf_idx++) {
            for (size_t elem_idx = 0; elem_idx < recv_counts[op.comm_rank]; elem_idx++) {
                op.recv_bufs[buf_idx][offsets[op.comm_rank] + elem_idx] = op.comm_rank + buf_idx;
            }
        }
    }

    void run_derived(test_operation<T>& op) {
        void* send_buf;
        void* recv_buf;

        auto param = op.get_param();
        auto attr = ccl::create_operation_attr<ccl::allgatherv_attr>();

        for (auto buf_idx : op.buf_indexes) {
            op.prepare_attr(attr, buf_idx);
            send_buf = op.get_send_buf(buf_idx);
            recv_buf = op.get_recv_buf(buf_idx);

            op.events.push_back(
                ccl::allgatherv((param.place_type == PLACE_IN) ? recv_buf : send_buf,
                                recv_counts[op.comm_rank],
                                recv_buf,
                                recv_counts,
                                op.datatype,
                                global_data::instance().comms[0],
                                attr));
        }
    }
};

RUN_METHOD_DEFINITION(allgatherv_test);
TEST_CASES_DEFINITION(allgatherv_test);
MAIN_FUNCTION();
