#include "base.hpp"

template <typename T>
class AlltoAllTest : public BaseTest<T>
{
public:
    int Check(TypedTestParam<T>& param)
    {
        CHECK_SELF_COMM(param);
        for (size_t idx = 0; idx < param.processCount * param.elemCount; idx++)
        {
            T expected = (T)(param.processIdx);

            if (param.recvBuf[idx] != expected)
            {
                sprintf(this->errMessage, "[%zu] got recvBuf[%zu] = %f, but expected = %f\n",
                                          param.processIdx, idx, (double)param.recvBuf[idx], (double)expected);
                return TEST_FAILURE;
            }
        }
        return TEST_SUCCESS;
    }

    int Run(TypedTestParam<T>& param)
    {
        for (size_t i = 0; i < param.processCount; i++)
        {
            for (size_t j = 0; j < param.elemCount; j++)
            {
                param.sendBuf[i * param.elemCount + j] = i;
                if (param.GetPlaceType() != PT_IN) param.recvBuf[i * param.elemCount + j] = (T)SOME_VALUE;
            }
        }

        CommReq* req = param.dist->AlltoAll(param.sendBuf, param.elemCount, param.recvBuf,
                                            (DataType)param.GetDataType(), (GroupType)param.GetGroupType());


        param.CompleteRequest(req);
        int result = Check(param);
        return result;
    }
};


RUN_METHOD_DEFINITION(AlltoAllTest);
TEST_CASES_DEFINITION(AlltoAllTest);
MAIN_FUNCTION();