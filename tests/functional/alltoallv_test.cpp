#define COLL_NAME "CCL_ALLTOALLV"

#include "base_impl.hpp"

template <typename T>
class alltoallv_test : public base_test<T> {
public:
    std::vector<size_t> send_counts;
    std::vector<size_t> recv_counts;

    int check(typed_test_param<T>& param) {
        for (size_t buf_idx = 0; buf_idx < param.buffer_count; buf_idx++) {
            size_t elem_idx = 0;
            for (int rank = 0; rank < param.comm_size; rank++) {
                T expected = static_cast<T>(rank + buf_idx);
                for (size_t idx = 0; idx < recv_counts[rank]; idx++) {
                    if (base_test<T>::check_error(param, expected, buf_idx, elem_idx))
                        return TEST_FAILURE;
                    elem_idx++;
                }
            }
        }
        return TEST_SUCCESS;
    }

    void alloc_buffers(typed_test_param<T>& param) {
        send_counts.resize(param.comm_size);
        recv_counts.resize(param.comm_size);
        if (param.test_conf.place_type == PT_IN) {
            /*
               Specifying the in-place option indicates that
               the same amount and type of data is sent and received
               between any two processes in the group of the communicator.
               Different pairs of processes can exchange different amounts of data.
               https://docs.microsoft.com/en-us/message-passing-interface/mpi-alltoallv-function#remarks
             */
            for (int rank = 0; rank < param.comm_size; rank++) {
                size_t common_size = (param.comm_rank + rank) * (param.elem_count / 4);
                recv_counts[rank] = ((common_size > param.elem_count) || (common_size == 0))
                                        ? param.elem_count
                                        : common_size;
                send_counts[rank] = recv_counts[rank];
            }
        }
        else {
            bool is_even_rank = (param.comm_rank % 2 == 0) ? true : false;
            size_t send_count = (is_even_rank) ? (param.elem_count / 2) : param.elem_count;
            for (int rank = 0; rank < param.comm_size; rank++) {
                int is_even_peer = (rank % 2 == 0) ? true : false;
                send_counts[rank] = send_count;
                recv_counts[rank] = (is_even_peer) ? (param.elem_count / 2) : param.elem_count;
            }
        }
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
        auto attr = ccl::create_operation_attr<ccl::alltoallv_attr>();
        ccl::datatype datatype = get_ccl_lib_datatype(test_conf);

        for (size_t buf_idx = 0; buf_idx < param.buffer_count; buf_idx++) {
            size_t new_idx = param.buf_indexes[buf_idx];
            param.prepare_coll_attr(attr, param.buf_indexes[buf_idx]);

            send_buf = param.get_send_buf(new_idx);
            recv_buf = param.get_recv_buf(new_idx);

            param.reqs[buf_idx] =
                ccl::alltoallv((test_conf.place_type == PT_IN) ? recv_buf : send_buf,
                               send_counts,
                               recv_buf,
                               recv_counts,
                               datatype,
                               GlobalData::instance().comms[0],
                               attr);
        }
    }
};

RUN_METHOD_DEFINITION(alltoallv_test);
TEST_CASES_DEFINITION(alltoallv_test);
MAIN_FUNCTION();
