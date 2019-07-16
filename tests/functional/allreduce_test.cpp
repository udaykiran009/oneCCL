#define TEST_ICCL_REDUCE
#define Collective_Name "ICCL_ALLREDUCE_ALGO"

#include "base.hpp"
#include <functional>
#include <vector>
#include <chrono>


template < typename T > class AllReduceTest:public BaseTest < T > {
public:
    int Check(TypedTestParam < T > &param) {
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
            for (idx = 0; idx < param.bufferCount; idx++) {
                this->Init(param, idx);
                param.req[Buffers[idx]] = (param.GetPlaceType() == PT_IN) ?
                    param.global_comm.allreduce(param.recvBuf[Buffers[idx]].data(), param.recvBuf[Buffers[idx]].data(), param.elemCount,
                                  (iccl::data_type) param.GetDataType(),(iccl::reduction) param.GetReductionName(), &param.coll_attr) :
                    param.global_comm.allreduce(param.sendBuf[Buffers[idx]].data(), param.recvBuf[Buffers[idx]].data(), param.elemCount,
                                  (iccl::data_type) param.GetDataType(),(iccl::reduction) param.GetReductionName(), &param.coll_attr);
            }
            param.DefineCompletionOrderAndComplete();
            result += Check(param);
        }
            return result;
    }
};

RUN_METHOD_DEFINITION(AllReduceTest);
TEST_CASES_DEFINITION(AllReduceTest);
MAIN_FUNCTION();
