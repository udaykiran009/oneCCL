#define ALGO_SELECTION_ENV "CCL_ALLGATHERV"

#include "base_impl.hpp"

template <typename T>
class allgatherv_test : public base_test<T> {
public:
    std::vector<size_t> recv_counts;
    std::vector<size_t> offsets;

    int check(typed_test_param<T>& param) {
        for (size_t buf_idx = 0; buf_idx < param.buffer_count; buf_idx++) {
            for (int rank = 0; rank < param.comm_size; rank++) {
                for (size_t elem_idx = 0; elem_idx < recv_counts[rank]; elem_idx++) {
                    size_t idx = offsets[rank] + elem_idx;
                    T expected = static_cast<T>(rank + buf_idx);
                    if (base_test<T>::check_error(param, expected, buf_idx, idx)) {
                        return TEST_FAILURE;
                    }
                }
            }
        }
        return TEST_SUCCESS;
    }

    void alloc_buffers(typed_test_param<T>& param) {
        recv_counts.resize(param.comm_size);
        offsets.resize(param.comm_size);
        recv_counts[0] = param.elem_count;
        offsets[0] = 0;

        for (int rank = 1; rank < param.comm_size; rank++) {
            recv_counts[rank] =
                ((int)param.elem_count > rank) ? param.elem_count - rank : param.elem_count;
            offsets[rank] = recv_counts[rank - 1] + offsets[rank - 1];
        }

        if (param.test_conf.place_type == PT_OOP) {
            size_t total_recv_count = std::accumulate(recv_counts.begin(), recv_counts.end(), 0);
            for (size_t buf_idx = 0; buf_idx < param.buffer_count; buf_idx++) {
                param.recv_buf[buf_idx].resize(total_recv_count);
                if (is_lp_datatype(param.test_conf.datatype)) {
                    param.recv_buf_lp[buf_idx].resize(total_recv_count);
                }
            }
        }
    }

    void fill_send_buffers(typed_test_param<T>& param) {
        for (size_t buf_idx = 0; buf_idx < param.buffer_count; buf_idx++) {
            for (size_t elem_idx = 0; elem_idx < recv_counts[param.comm_rank]; elem_idx++) {
                param.send_buf[buf_idx][elem_idx] = param.comm_rank + buf_idx;
            }
        }
    }

    void fill_recv_buffers(typed_test_param<T>& param) {
        if (param.test_conf.place_type != PT_IN)
            return;

        /* in case of in-place i-th rank already has result in i-th block of send buffer */
        for (size_t buf_idx = 0; buf_idx < param.buffer_count; buf_idx++) {
            for (size_t elem_idx = 0; elem_idx < recv_counts[param.comm_rank]; elem_idx++) {
                param.recv_buf[buf_idx][offsets[param.comm_rank] + elem_idx] =
                    param.comm_rank + buf_idx;
            }
        }
    }

    void run_derived(typed_test_param<T>& param) {
        void* send_buf;
        void* recv_buf;

        const ccl_test_conf& test_conf = param.get_conf();
        auto attr = ccl::create_operation_attr<ccl::allgatherv_attr>();
        ccl::datatype datatype = get_ccl_lib_datatype(test_conf);

        for (auto buf_idx : param.buf_indexes) {
            param.prepare_coll_attr(attr, buf_idx);
            send_buf = param.get_send_buf(buf_idx);
            recv_buf = param.get_recv_buf(buf_idx);

            param.events.push_back(
                ccl::allgatherv((test_conf.place_type == PT_IN) ? recv_buf : send_buf,
                                recv_counts[param.comm_rank],
                                recv_buf,
                                recv_counts,
                                datatype,
                                GlobalData::instance().comms[0],
                                attr));
        }
    }
};

RUN_METHOD_DEFINITION(allgatherv_test);
TEST_CASES_DEFINITION(allgatherv_test);
MAIN_FUNCTION();
