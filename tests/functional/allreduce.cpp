#include "base.hpp"

template <typename T>
class AllReduceTest : public BaseTest<T>
{
public:
    int Check(TypedTestParam<T>& param)
    {
        for (size_t i = 0; i < param.elemCount; i++)
        {
            T expected = (SELF_COMM(param)) ? 
                         param.sendBuf[i] :
                         ((param.processCount * (param.processCount - 1) / 2) + (i * param.processCount));
            if (param.recvBuf[i] != expected)
            {
                sprintf(this->errMessage, "[%zu] got recvBuf[%zu] = %f, but expected = %f\n",
                                          param.processIdx, i, (double)param.recvBuf[i], (double)expected);
                return TEST_FAILURE;
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

        CommReq* req = param.dist->AllReduce(param.sendBuf, param.recvBuf, param.elemCount,
                                            (DataType)param.GetDataType(),
                                            RT_SUM, (GroupType)param.GetGroupType());
        param.CompleteRequest(req);
        int result = Check(param);
        return result;
    }
};

RUN_METHOD_DEFINITION(AllReduceTest);
TEST_CASES_DEFINITION(AllReduceTest);
MAIN_FUNCTION();