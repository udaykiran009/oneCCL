#define TEST_CCL_REDUCE
#define Collective_Name "CCL_REDUCE_ALGO"

#include "base.hpp"
#include <functional>
#include <vector>
#include <chrono>

// template <typename T>
// ccl_status_t do_reduction_null(const void* in_buf, size_t in_count, void* inout_buf,
                               // size_t* out_count, const void** ctx, ccl_datatype_t dtype)
// {
    // size_t idx;
    // if (out_count) *out_count = in_count;
            // for (idx = 0; idx < in_count; idx++)
            // {
                // ((T*)inout_buf)[idx] = (T)0;
            // }
    // return ccl_status_success;
// }
// template <typename T>
// ccl_status_t do_reduction_custom(const void* in_buf, size_t in_count, void* inout_buf,
                               // size_t* out_count, const void** ctx, ccl_datatype_t dtype)
// {
    // size_t idx;
    // if (out_count) *out_count = in_count;
            // for (idx = 0; idx < in_count; idx++)
            // {
                // ((T*)inout_buf)[idx] += ((T*)in_buf)[idx];
            // }
    // return ccl_status_success;
// }
// template <typename T>
// int set_custom_reduction (TypedTestParam <T> &param){
    // TestReductionType customFuncName = param.GetReductionName();
    // switch (customFuncName) {
        // case RT_CUSTOM:
            // param.coll_attr.reduction_fn = do_reduction_custom<T>;
            // break;
        // case RT_CUSTOM_NULL:
            // param.coll_attr.reduction_fn = do_reduction_null<T>;
            // break;
        // default:
            // return TEST_FAILURE;

    // }
    // return TEST_SUCCESS;
// }
template < typename T > class ReduceTest:public BaseTest < T > {
public:
    int Check(TypedTestParam < T > &param) {
        if (param.processIdx == ROOT_PROCESS_IDX) {
            for (size_t j = 0; j < param.bufferCount; j++) {
                for (size_t i = 0; i < param.elemCount; i++) {
                    if (param.GetReductionName() == RT_SUM) {
                        T expected =
                            ((param.processCount * (param.processCount - 1) / 2) +
                            ((i + j) * param.processCount));
                        if (param.recvBuf[j][i] != expected) {
                            sprintf(this->errMessage, "[%zu] sent sendBuf[%zu][%zu] = %f, got recvBuf[%zu][%zu] = %f, but expected = %f\n",
                                param.processIdx, j, i, (double) param.sendBuf[j][i], j, i, (double) param.recvBuf[j][i], (double) expected);
                            return TEST_FAILURE;
                        }
                    }
                    if (param.GetReductionName() == RT_MAX) {
                        T expected = get_expected_max<T>(i, j, param.processCount);
                        if (param.recvBuf[j][i] != expected) {
                            sprintf(this->errMessage, "[%zu] sent sendBuf[%zu][%zu] = %f, got recvBuf[%zu][%zu] = %f, but expected = %f\n",
                                param.processIdx, j, i, (double) param.sendBuf[j][i], j, i, (double) param.recvBuf[j][i], (double) expected);
                            return TEST_FAILURE;
                        }
                    }
                    if (param.GetReductionName() == RT_MIN) {
                        T expected = get_expected_min<T>(i, j, param.processCount);
                        if (param.recvBuf[j][i] != expected) {
                            sprintf(this->errMessage, "[%zu] sent sendBuf[%zu][%zu] = %f, got recvBuf[%zu][%zu] = %f, but expected = %f\n",
                                param.processIdx, j, i, (double) param.sendBuf[j][i], j, i, (double) param.recvBuf[j][i], (double) expected);
                            return TEST_FAILURE;
                        }
                    }
                    if (param.GetReductionName() == RT_PROD) {
                        T expected = 1;
                        for (size_t k = 0; k < param.processCount; k++) {
                            expected *= i + j + k;
                        }
                        if (param.recvBuf[j][i] != expected) {
                            sprintf(this->errMessage, "[%zu] sent sendBuf[%zu][%zu] = %f, got recvBuf[%zu][%zu] = %f, but expected = %f\n",
                                param.processIdx, j, i, (double) param.sendBuf[j][i], j, i, (double) param.recvBuf[j][i], (double) expected);
                            return TEST_FAILURE;
                        }
                    }
                    // if (param.GetReductionName() == RT_CUSTOM) {
                        // T expected =
                            // ((param.processCount * (param.processCount - 1) / 2) +
                            // (i * param.processCount));
                        // if (param.recvBuf[j][i] != expected) {
                            // sprintf(this->errMessage, "[%zu] sent sendBuf[%zu][%zu] = %f, got recvBuf[%zu][%zu] = %f, but expected = %f\n",
                                // param.processIdx, j, i, (double) param.sendBuf[j][i], j, i, (double) param.recvBuf[j][i], (double) expected);
                            // return TEST_FAILURE;
                        // }
                    // }
                    // else if (param.GetReductionName() == RT_CUSTOM_NULL) {
                        // T expected = 0;
                        // if (param.recvBuf[j][i] != expected) {
                            // sprintf(this->errMessage, "[%zu] sent sendBuf[%zu][%zu] = %f, got recvBuf[%zu][%zu] = %f, but expected = %f\n",
                                // param.processIdx, j, i, (double) param.sendBuf[j][i], j, i, (double) param.recvBuf[j][i], (double) expected);
                            // return TEST_FAILURE;
                        // }
                    // }
                }
            }
        }
        return TEST_SUCCESS;
    }

    int Run(TypedTestParam < T > &param) {
        size_t result = 0;
        for (size_t iter = 0; iter < 2; iter++) {
            SHOW_ALGO(Collective_Name);
            this->FillBuffers (param);
            this->SwapBuffers(param, iter);
            size_t idx = 0;
            size_t* Buffers = param.DefineStartOrder();
            // if (param.GetReductionType() == ccl_reduction_custom) {
                // if (set_custom_reduction<T>(param))
                    // return TEST_FAILURE;
            // }
            for (idx = 0; idx < param.bufferCount; idx++) {
                this->Init(param, idx);
                param.req[Buffers[idx]] = (param.GetPlaceType() == PT_IN) ?
                        param.global_comm.reduce(param.recvBuf[Buffers[idx]].data(), param.recvBuf[Buffers[idx]].data(), param.elemCount,
                                                (ccl::data_type) param.GetDataType(), (ccl::reduction) param.GetReductionName(),
                                                ROOT_PROCESS_IDX, &param.coll_attr) :
                        param.global_comm.reduce(param.sendBuf[Buffers[idx]].data(), param.recvBuf[Buffers[idx]].data(), param.elemCount,
                                                (ccl::data_type) param.GetDataType(), (ccl::reduction) param.GetReductionName(),
                                                ROOT_PROCESS_IDX, &param.coll_attr);
            }
            param.DefineCompletionOrderAndComplete();
            result += Check(param);
        }
        return result;
    }
};

RUN_METHOD_DEFINITION(ReduceTest);
TEST_CASES_DEFINITION(ReduceTest);
MAIN_FUNCTION();
