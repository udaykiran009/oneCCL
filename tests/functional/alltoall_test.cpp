#define TEST_CCL_REDUCE
#define Collective_Name "CCL_ALLTOALL"

#include "base.hpp"
#include <functional>
#include <vector>
#include <chrono>


template <typename T> class AlltoallTest : public BaseTest <T> {
public:
    int Check(TypedTestParam<T>& param) {
        for (size_t j = 0; j < param.bufferCount; j++) {
            for (size_t k = 0; k < param.processCount; k++) {
                for (size_t i = 0; i < param.elemCount; i++) {
                    T expected = static_cast<T>(k);
                    if (param.recvBuf[j][(param.elemCount * k) + i] != expected) {
                        sprintf(this->errMessage, "[%zu] got recvBuf[%zu][%zu]  = %f, but expected = %f\n",
                            param.processIdx, j, (param.elemCount * k) + i, (double) param.recvBuf[j][(param.elemCount * k) + i], (double) expected);
                        return TEST_FAILURE;
                    }
                }
            }
        }
        return TEST_SUCCESS;
    }
    void FillBuffers(TypedTestParam<T>& param){
        for (size_t i = 0; i < param.bufferCount; i++)
        {
            param.recvBuf[i].resize(param.elemCount * param.processCount * sizeof(T));
            param.sendBuf[i].resize(param.elemCount * param.processCount * sizeof(T));
        }
        for (size_t j = 0; j < param.bufferCount; j++) {
            for (size_t i = 0; i < param.processCount * param.elemCount; i++) {
                param.sendBuf[j][i] = param.processIdx;
                if (param.GetPlaceType() == PT_OOP)
                {
                    param.recvBuf[j][i] = static_cast<T>SOME_VALUE;
                }
            }
        }
        if (param.GetPlaceType() != PT_OOP)
        {
            for (size_t i = 0; i < param.bufferCount; i++)
            {
                param.recvBuf[i] = param.sendBuf[i];
            }
        }
    }
    int Run(TypedTestParam<T>& param) {
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
                    param.global_comm->alltoall(param.recvBuf[Buffers[idx]].data(), param.recvBuf[Buffers[idx]].data(), param.elemCount,
                                  &param.coll_attr, param.GetStream()) :
                    param.global_comm->alltoall(param.sendBuf[Buffers[idx]].data(), param.recvBuf[Buffers[idx]].data(), param.elemCount,
                                  &param.coll_attr, param.GetStream());
            }
            param.DefineCompletionOrderAndComplete();
            result += Check(param);
        }
            return result;
    }
};

RUN_METHOD_DEFINITION(AlltoallTest);
TEST_CASES_DEFINITION(AlltoallTest);
MAIN_FUNCTION();
