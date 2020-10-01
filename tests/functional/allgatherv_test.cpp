#define TEST_CCL_ALLGATHERV

#define COLL_NAME "CCL_ALLGATHERV"

#include "base_impl.hpp"

template <typename T>
class allgatherv_test : public base_test<T> {
public:
    std::vector<size_t> recv_counts;
    std::vector<size_t> offsets;

    int check(typed_test_param<T>& param) {
        for (size_t buf_idx = 0; buf_idx < param.buffer_count; buf_idx++) {
            for (size_t elem_idx = 0; elem_idx < param.process_count; elem_idx++) {
                for (size_t recv_count_idx = 0; recv_count_idx < recv_counts[elem_idx];
                     recv_count_idx++) {
                    size_t idx = offsets[elem_idx] + recv_count_idx;
                    T expected = static_cast<T>(elem_idx + recv_count_idx);
                    if (base_test<T>::check_error(param, expected, buf_idx, idx)) {
                        return TEST_FAILURE;
                    }
                }
            }
        }
        return TEST_SUCCESS;
    }

    void alloc_buffers(typed_test_param<T>& param) {
        base_test<T>::alloc_buffers(param);
        recv_counts.resize(param.process_count);
        offsets.resize(param.process_count);
        offsets[0] = 0;
        recv_counts[0] = param.elem_count;

        if (param.test_conf.place_type == PT_OOP) {
            for (size_t elem_idx = 0; elem_idx < param.buffer_count; elem_idx++) {
                /* each buffer is different size */
                param.recv_buf[elem_idx].resize(param.elem_count * param.process_count);
                if (param.test_conf.datatype == DT_BF16) {
                    param.recv_buf_bf16[elem_idx].resize(param.elem_count * param.process_count);
                }
            }
        }
    }

    void fill_buffers(typed_test_param<T>& param) {
        for (size_t proc_idx = 1; proc_idx < param.process_count; proc_idx++) {
            recv_counts[proc_idx] =
                (param.elem_count > proc_idx) ? param.elem_count - proc_idx : param.elem_count;
            offsets[proc_idx] = recv_counts[proc_idx - 1] + offsets[proc_idx - 1];
        }

        for (size_t buf_idx = 0; buf_idx < param.buffer_count; buf_idx++) {
            for (size_t elem_idx = 0; elem_idx < param.process_count; elem_idx++) {
                for (size_t recv_count_idx = 0; recv_count_idx < recv_counts[elem_idx];
                     recv_count_idx++) {
                    param.send_buf[buf_idx][offsets[elem_idx] + recv_count_idx] =
                        param.process_idx + elem_idx + recv_count_idx;
                    if (param.test_conf.place_type == PT_OOP) {
                        param.recv_buf[buf_idx][offsets[elem_idx] + recv_count_idx] =
                            static_cast<T>(SOME_VALUE);
                        if (param.test_conf.datatype == DT_BF16) {
                            param.recv_buf_bf16[buf_idx][offsets[elem_idx] + recv_count_idx] =
                                static_cast<short>(SOME_VALUE);
                        }
                    }
                }
            }

            /* in case of in-place i-th process already has result in i-th block of send buffer */
            if (param.test_conf.place_type != PT_OOP) {
                param.recv_buf[buf_idx] = param.send_buf[buf_idx];
                for (size_t recv_count_idx = 0; recv_count_idx < recv_counts[param.process_idx];
                     recv_count_idx++) {
                    param.send_buf[buf_idx][offsets[param.process_idx] + recv_count_idx] =
                        param.process_idx + recv_count_idx;
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
        size_t count = recv_counts[param.process_idx];

        const ccl_test_conf& test_conf = param.get_conf();

        auto attr = ccl::create_operation_attr<ccl::allgatherv_attr>();

        ccl::datatype datatype = get_ccl_lib_datatype(test_conf);

        for (size_t buf_idx = 0; buf_idx < param.buffer_count; buf_idx++) {
            size_t new_idx = param.buf_indexes[buf_idx];
            param.prepare_coll_attr(attr, param.buf_indexes[buf_idx]);

            send_buf = param.get_send_buf(new_idx);
            recv_buf = param.get_recv_buf(new_idx);

            param.reqs[buf_idx] = ccl::allgatherv(
                (test_conf.place_type == PT_IN) ? recv_buf : send_buf,
                count,
                recv_buf,
                recv_counts,
                datatype,
                GlobalData::instance().comms[0],
                attr);
        }
    }
};

RUN_METHOD_DEFINITION(allgatherv_test);
TEST_CASES_DEFINITION(allgatherv_test);
MAIN_FUNCTION();
