#include "base.hpp"

template <typename T>
class ReduceTest : public BaseTest<T>
{
public:
    int Check(TypedTestParam<T>& param)
    {
        CHECK_SELF_COMM(param);

        if (param.processIdx == ROOT_PROCESS_IDX)
        {
            for (size_t i = 0; i < param.elemCount; i++)
            {
                T expected = ((param.processCount * (param.processCount - 1) / 2) + (i * param.processCount));
                if (param.recvBuf[i] != expected)
                {
                    sprintf(this->errMessage, "[%zu] got recvBuf[%zu] = %f, but expected = %f\n",
                                              param.processIdx, i, (double)param.recvBuf[i], (double)expected);
                    return TEST_FAILURE;
                }
            }
        }
        return TEST_SUCCESS;
    }

    int Run(TypedTestParam<T>& param)
    {
        for (size_t i = 0; i < param.elemCount; i++)
        {
            param.sendBuf[i] = param.processIdx + i;
            if (param.GetPlaceType() != PT_IN) param.recvBuf[i] = (T)SOME_VALUE;
        }

        CommReq* req = param.dist->Reduce(param.sendBuf, param.recvBuf, param.elemCount, (DataType)param.GetDataType(),
                                          RT_SUM, ROOT_PROCESS_IDX, (GroupType)param.GetGroupType());
        param.CompleteRequest(req);
        int result = Check(param);
        return result;
    }
};

RUN_METHOD_DEFINITION(ReduceTest);
TEST_CASES_DEFINITION(ReduceTest);
MAIN_FUNCTION();
