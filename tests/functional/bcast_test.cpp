#include "base.hpp"
#include <functional>
#include <vector>
#include <chrono>

#define Collective_Name "CCL_BCAST_ALGO"

template <typename T> class BcastTest:public BaseTest <T> {
public:
    int Check(TypedTestParam <T> &param) {
        for (size_t j = 0; j < param.bufferCount; j++) {
            for (size_t i = 0; i < param.elemCount; i++) {
                if (param.sendBuf[j][i] != static_cast<T>(i)) {
                    sprintf(this->errMessage, "[%zu] got sendBuf[%zu][%zu] = %f, but expected = %f\n",
                        param.processIdx, j, i, (double)param.sendBuf[j][i], (double)i);
                    return TEST_FAILURE;
                }
            }
        }
        return TEST_SUCCESS;
    }
    void FillBuffers(TypedTestParam <T> &param){
        for (size_t j = 0; j < param.bufferCount; j++) {
            for (size_t i = 0; i < param.elemCount; i++) {
                if (param.processIdx == ROOT_PROCESS_IDX)
                    param.sendBuf[j][i] = i;
                else
                    param.sendBuf[j][i] = static_cast<T>(SOME_VALUE);
            }
        }
    }
    int Run(TypedTestParam <T> &param) {
        size_t result = 0;
        for (size_t iter = 0; iter < 2; iter++) {
            SHOW_ALGO(Collective_Name);
            this->FillBuffers(param);
            this->SwapBuffers(param, iter);
            size_t idx = 0;
            size_t* Buffers = param.DefineStartOrder();
            for (idx = 0; idx < param.bufferCount; idx++) {
                this->Init(param, idx);
                param.req[Buffers[idx]] = param.global_comm.bcast(param.sendBuf[Buffers[idx]].data(), param.elemCount,
                                  (ccl::data_type) param.GetDataType(), ROOT_PROCESS_IDX, &param.coll_attr);
            }
            param.DefineCompletionOrderAndComplete();
            result += Check(param);
        }
        return result;
    }
};

RUN_METHOD_DEFINITION(BcastTest);
TEST_CASES_DEFINITION(BcastTest);
MAIN_FUNCTION();
