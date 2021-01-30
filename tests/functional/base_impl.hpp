#pragma once

#include <math.h>

#include "base.hpp"
#include "lp.hpp"

#define FIRST_FP_COEFF  (0.1)
#define SECOND_FP_COEFF (0.01)

template <typename T>
template <class coll_attr_type>
void typed_test_param<T>::prepare_coll_attr(coll_attr_type& coll_attr, size_t idx) {
    coll_attr.template set<ccl::operation_attr_id::priority>(generate_priority_value(idx));
    coll_attr.template set<ccl::operation_attr_id::to_cache>(
        test_conf.cache_type == CT_CACHE_1 ? true : false);

    char* test_unordered_coll = getenv("CCL_UNORDERED_COLL");
    if (test_unordered_coll && atoi(test_unordered_coll) == 1) {
        coll_attr.template set<ccl::operation_attr_id::synchronous>(false);
    }
    else {
        coll_attr.template set<ccl::operation_attr_id::synchronous>(
            test_conf.sync_type == SNCT_SYNC_1 ? true : false);
    }

    match_id = create_match_id(idx);
    coll_attr.template set<ccl::operation_attr_id::match_id>(match_id);
}

template <typename T>
std::string typed_test_param<T>::create_match_id(size_t buf_idx) {
    return (std::to_string(buf_idx) + std::to_string(comm_size) + std::to_string(elem_count) +
            std::to_string(buffer_count) + std::to_string(test_conf.reduction) +
            std::to_string(test_conf.sync_type) + std::to_string(test_conf.cache_type) +
            std::to_string(test_conf.size_type) + std::to_string(test_conf.datatype) +
            std::to_string(test_conf.completion_type) + std::to_string(test_conf.place_type) +
            std::to_string(test_conf.start_order_type) +
            std::to_string(test_conf.complete_order_type) + std::to_string(test_conf.prolog_type) +
            std::to_string(test_conf.epilog_type));
}

template <typename T>
bool typed_test_param<T>::complete_request(ccl::event& e) {
    if (test_conf.completion_type == CMPT_TEST) {
        return e.test();
    }
    else if (test_conf.completion_type == CMPT_WAIT) {
        e.wait();
        return true;
    }
    else {
        ASSERT(0, "unexpected completion type %d", test_conf.completion_type);
        return false;
    }
}

template <typename T>
void typed_test_param<T>::define_start_order(std::default_random_engine& rand_engine) {
    std::iota(buf_indexes.begin(), buf_indexes.end(), 0);
    if (test_conf.start_order_type == ORDER_INDIRECT) {
        std::reverse(buf_indexes.begin(), buf_indexes.end());
    }
    else if (test_conf.start_order_type == ORDER_RANDOM) {
        char* test_unordered_coll = getenv("CCL_UNORDERED_COLL");
        if (test_unordered_coll && atoi(test_unordered_coll) == 1) {
            std::shuffle(buf_indexes.begin(), buf_indexes.end(), rand_engine);
        }
    }
}

template <typename T>
bool typed_test_param<T>::complete() {
    size_t idx, msg_idx;
    size_t completions = 0;
    std::vector<bool> msg_completions(buffer_count, false);

    while (completions < buffer_count) {
        for (idx = 0; idx < buffer_count; idx++) {
            if (test_conf.complete_order_type == ORDER_DIRECT) {
                msg_idx = idx;
            }
            else if (test_conf.complete_order_type == ORDER_INDIRECT) {
                msg_idx = (buffer_count - idx - 1);
            }
            else if (test_conf.complete_order_type == ORDER_RANDOM) {
                msg_idx = rand() % buffer_count;
            }
            else {
                msg_idx = idx;
            }

            if (msg_completions[msg_idx])
                continue;

            if (complete_request(events[msg_idx])) {
                completions++;
                msg_completions[msg_idx] = true;
            }
        }
    }

    events.clear();

    return TEST_SUCCESS;
}

template <typename T>
void typed_test_param<T>::change_buffer_pointers() {
    char* test_dynamic_pointer = getenv("TEST_DYNAMIC_POINTER");
    if (test_dynamic_pointer && atoi(test_dynamic_pointer) == 1) {
        /*
            create deep copy of vector with buffers and swap it with original one
            as result buffers in updated vector will have original content
            but in new memory locations
        */
        if (comm_rank % 2) {
            std::vector<std::vector<T>>(send_buf.begin(), send_buf.end()).swap(send_buf);
        }
        else {
            std::vector<std::vector<T>>(recv_buf.begin(), recv_buf.end()).swap(recv_buf);
        }
    }
}

template <typename T>
size_t typed_test_param<T>::generate_priority_value(size_t buf_idx) {
    return buf_idx++;
}

template <typename T>
void typed_test_param<T>::print(std::ostream& output) {
    output << "test conf:\n"
           << test_conf << "\ncomm_size: " << comm_size << "\ncomm_rank: " << comm_rank
           << "\nelem_count: " << elem_count << "\nbuffer_count: " << buffer_count
           << "\nmatch_id: " << match_id << "\n-------------\n"
           << std::endl;
}

template <typename T>
base_test<T>::base_test() {
    global_comm_rank = GlobalData::instance().comms[0].rank();
    global_comm_size = GlobalData::instance().comms[0].size();
    memset(err_message, '\0', ERR_MESSAGE_MAX_LEN);
    rand_engine = std::default_random_engine{ rand_device() };
}

template <typename T>
int base_test<T>::check_error(typed_test_param<T>& param,
                              T expected,
                              size_t buf_idx,
                              size_t elem_idx) {
    double max_error = 0;
    double precision = 0;

    if (param.test_conf.datatype == DT_FLOAT16) {
        precision = 2 * FP16_PRECISION;
    }
    else if (param.test_conf.datatype == DT_FLOAT32) {
        precision = FP32_PRECISION;
    }
    else if (param.test_conf.datatype == DT_FLOAT64) {
        precision = FP64_PRECISION;
    }
    else if (param.test_conf.datatype == DT_BFLOAT16) {
        precision = 2 * BF16_PRECISION;
    }

    if (precision) {
        /* https://www.mcs.anl.gov/papers/P4093-0713_1.pdf */
        double log_base2 = log(param.comm_size) / log(2);
        double g = (log_base2 * precision) / (1 - (log_base2 * precision));
        max_error = g * expected;
    }

    if (fabs(max_error) < fabs((double)expected - (double)param.recv_buf[buf_idx][elem_idx])) {
        printf("[%d] got param.recvBuf[%zu][%zu] = %0.7f, but expected = %0.7f, "
               "max_error = %0.10f, precision = %0.7f\n",
               param.comm_rank,
               buf_idx,
               elem_idx,
               (double)param.recv_buf[buf_idx][elem_idx],
               (double)expected,
               (double)max_error,
               precision);
        return TEST_FAILURE;
    }

    return TEST_SUCCESS;
}

template <typename T>
void base_test<T>::alloc_buffers_base(typed_test_param<T>& param) {
    param.send_buf.resize(param.buffer_count);
    param.recv_buf.resize(param.buffer_count);
    if (is_lp_datatype(param.test_conf.datatype)) {
        param.send_buf_lp.resize(param.buffer_count);
        param.recv_buf_lp.resize(param.buffer_count);
    }

    for (size_t buf_idx = 0; buf_idx < param.buffer_count; buf_idx++) {
        param.send_buf[buf_idx].resize(param.elem_count * param.comm_size);
        param.recv_buf[buf_idx].resize(param.elem_count * param.comm_size);

        if (is_lp_datatype(param.test_conf.datatype)) {
            param.send_buf_lp[buf_idx].resize(param.elem_count * param.comm_size);
            param.recv_buf_lp[buf_idx].resize(param.elem_count * param.comm_size);
        }
    }
}

template <typename T>
void base_test<T>::alloc_buffers(typed_test_param<T>& param) {}

template <typename T>
void base_test<T>::fill_send_buffers_base(typed_test_param<T>& param) {
    if (!is_lp_datatype(param.test_conf.datatype))
        return;

    for (size_t buf_idx = 0; buf_idx < param.buffer_count; buf_idx++) {
        std::fill(
            param.send_buf_lp[buf_idx].begin(), param.send_buf_lp[buf_idx].end(), (T)SOME_VALUE);
    }
}

template <typename T>
void base_test<T>::fill_send_buffers(typed_test_param<T>& param) {
    for (size_t buf_idx = 0; buf_idx < param.buffer_count; buf_idx++) {
        for (size_t elem_idx = 0; elem_idx < param.send_buf[buf_idx].size(); elem_idx++) {
            param.send_buf[buf_idx][elem_idx] = param.comm_rank + buf_idx;

            if (param.test_conf.reduction == RT_PROD) {
                param.send_buf[buf_idx][elem_idx] += 1;
            }
        }
    }
}

template <typename T>
void base_test<T>::fill_recv_buffers_base(typed_test_param<T>& param) {
    for (size_t buf_idx = 0; buf_idx < param.buffer_count; buf_idx++) {
        if (param.test_conf.place_type == PT_IN) {
            std::copy(param.send_buf[buf_idx].begin(),
                      param.send_buf[buf_idx].end(),
                      param.recv_buf[buf_idx].begin());
        }
        else {
            std::fill(
                param.recv_buf[buf_idx].begin(), param.recv_buf[buf_idx].end(), (T)SOME_VALUE);
        }
        if (is_lp_datatype(param.test_conf.datatype)) {
            std::fill(
                param.recv_buf_lp[buf_idx].begin(), param.recv_buf_lp[buf_idx].end(), SOME_VALUE);
        }
    }
}

template <typename T>
void base_test<T>::fill_recv_buffers(typed_test_param<T>& param) {}

template <typename T>
T base_test<T>::calculate_reduce_value(typed_test_param<T>& param,
                                       size_t buf_idx,
                                       size_t elem_idx) {
    T expected = 0;
    switch (param.test_conf.reduction) {
        case RT_SUM:
            expected = (param.comm_size * (param.comm_size - 1)) / 2 + param.comm_size * buf_idx;
            break;
        case RT_PROD:
            expected = 1;
            for (int rank = 0; rank < param.comm_size; rank++) {
                expected *= rank + buf_idx + 1;
            }
            break;
        case RT_MIN: expected = (T)(buf_idx); break;
        case RT_MAX: expected = (T)(param.comm_size - 1 + buf_idx); break;
        default: ASSERT(0, "unexpected reduction %d", param.test_conf.reduction); break;
    }
    return expected;
}

template <>
void base_test<float>::fill_send_buffers(typed_test_param<float>& param) {
    for (size_t buf_idx = 0; buf_idx < param.buffer_count; buf_idx++) {
        for (size_t elem_idx = 0; elem_idx < param.send_buf[buf_idx].size(); elem_idx++) {
            param.send_buf[buf_idx][elem_idx] =
                FIRST_FP_COEFF * param.comm_rank + SECOND_FP_COEFF * buf_idx;

            if (param.test_conf.reduction == RT_PROD) {
                param.send_buf[buf_idx][elem_idx] += 1;
            }
        }
    }
}

template <>
float base_test<float>::calculate_reduce_value(typed_test_param<float>& param,
                                               size_t buf_idx,
                                               size_t elem_idx) {
    float expected = 0;
    switch (param.test_conf.reduction) {
        case RT_SUM:
            expected = param.comm_size *
                       (FIRST_FP_COEFF * (param.comm_size - 1) / 2 + SECOND_FP_COEFF * buf_idx);
            break;
        case RT_PROD:
            expected = 1;
            for (int rank = 0; rank < param.comm_size; rank++) {
                expected *= FIRST_FP_COEFF * rank + SECOND_FP_COEFF * buf_idx + 1;
            }
            break;
        case RT_MIN: expected = SECOND_FP_COEFF * buf_idx; break;
        case RT_MAX:
            expected = FIRST_FP_COEFF * (param.comm_size - 1) + SECOND_FP_COEFF * buf_idx;
            break;
        default: ASSERT(0, "unexpected reduction %d", param.test_conf.reduction); break;
    }
    return expected;
}

template <typename T>
int base_test<T>::run(typed_test_param<T>& param) {
    size_t result = 0;

    char* algo = getenv(ALGO_SELECTION_ENV);
    if (algo)
        std::cout << ALGO_SELECTION_ENV << " = " << algo << "\n";
    std::cout << param.test_conf << "\n";

    for (size_t iter = 0; iter < ITER_COUNT; iter++) {
        try {
            alloc_buffers_base(param);
            alloc_buffers(param);

            fill_send_buffers_base(param);
            fill_send_buffers(param);

            fill_recv_buffers_base(param);
            fill_recv_buffers(param);

            if (iter > 0) {
                param.change_buffer_pointers();
            }

            param.define_start_order(rand_engine);

            if (is_lp_datatype(param.test_conf.datatype)) {
                make_lp_prologue(param, param.comm_size * param.elem_count);
            }

            run_derived(param);

            param.complete();

            if (is_lp_datatype(param.test_conf.datatype)) {
                make_lp_epilogue(param, param.comm_size * param.elem_count);
            }

            result += check(param);
        }
        catch (const std::exception& ex) {
            result += TEST_FAILURE;
            printf("WARNING! %s iter number: %zu", ex.what(), iter);
        }
    }

    return result;
}
