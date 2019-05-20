#define TEST_MLSL_REDUCE

#include "base.hpp"
#include <functional>
#include <vector>
#include <chrono>


template < typename T > class ReduceTest:public BaseTest < T > {
public:
    int Check(TypedTestParam < T > &param) {
        if (param.processIdx == ROOT_PROCESS_IDX) {
            for (size_t j = 0; j < param.bufferCount; j++) {
                for (size_t i = 0; i < param.elemCount; i++) {
                    if (param.GetReductionType() == RT_SUM) {
                        T expected =
                            ((param.processCount * (param.processCount - 1) / 2) +
                            (i * param.processCount));
                        if (param.recvBuf[j][i] != expected) {
                            sprintf(this->errMessage, "[%zu] sent sendBuf[%zu][%zu] = %f, got recvBuf[%zu][%zu] = %f, but expected = %f\n",
                                param.processIdx, j, i, (double) param.sendBuf[j][i], j, i, (double) param.recvBuf[j][i], (double) expected);
                            return TEST_FAILURE;
                        }
                    }

                    if (param.GetReductionType() == RT_MAX) {
                        T expected = BaseTest<T>::get_expected_max(i, param.processCount);
                        if (param.recvBuf[j][i] != expected) {
                            sprintf(this->errMessage, "[%zu] sent sendBuf[%zu][%zu] = %f, got recvBuf[%zu][%zu] = %f, but expected = %f\n",
                                param.processIdx, j, i, (double) param.sendBuf[j][i], j, i, (double) param.recvBuf[j][i], (double) expected);
                            return TEST_FAILURE;
                        }
                    }
                    if (param.GetReductionType() == RT_MIN) {
                        T expected = BaseTest<T>::get_expected_min(i, param.processCount);
                        if (param.recvBuf[j][i] != expected) {
                            sprintf(this->errMessage, "[%zu] sent sendBuf[%zu][%zu] = %f, got recvBuf[%zu][%zu] = %f, but expected = %f\n",
                                param.processIdx, j, i, (double) param.sendBuf[j][i], j, i, (double) param.recvBuf[j][i], (double) expected);
                            return TEST_FAILURE;
                        }
                    }
                    if (param.GetReductionType() == RT_PROD) {
                        T expected = 1;
                        for (size_t k = 0; k < param.processCount; k++) {
                            expected *= i + k;
                        }
                        if (param.recvBuf[j][i] != expected) {
                            sprintf(this->errMessage, "[%zu] sent sendBuf[%zu][%zu] = %f, got recvBuf[%zu][%zu] = %f, but expected = %f\n",
                                param.processIdx, j, i, (double) param.sendBuf[j][i], j, i, (double) param.recvBuf[j][i], (double) expected);
                            return TEST_FAILURE;
                        }
                    }
                }   
            }
        }
        return TEST_SUCCESS;
    }

    int Run(TypedTestParam < T > &param) {
        for (size_t j = 0; j < param.bufferCount; j++) {
            for (size_t i = 0; i < param.elemCount; i++) {
                param.sendBuf[j][i] = param.processIdx + i;
            }
        }
        size_t idx = 0;
        for (idx = 0; idx < param.bufferCount; idx++) { 
            BaseTest<T>::Init (param);
            param.req[idx] = param.global_comm.reduce(param.sendBuf[idx].data(), param.recvBuf[idx].data(), param.elemCount,
                (mlsl::data_type) param.GetDataType(),
                (mlsl::reduction) param.GetReductionType(), ROOT_PROCESS_IDX,
                &param.coll_attr);
        }
        for (idx = 0; idx < param.bufferCount; idx++) {
            param.CompleteRequest(param.req[idx]);
        }
        int result = Check(param);
        return result;
    }
};

RUN_METHOD_DEFINITION(ReduceTest);
TEST_CASES_DEFINITION(ReduceTest);
MAIN_FUNCTION();
