#include "base.hpp"

template <typename T>
class AllGatherTest : public BaseTest<T>
{
public:
    int Check(TypedTestParam<T>& param)
    {
        CHECK_SELF_COMM(param);

        for (size_t i = 0; i < param.processCount; i++)
        {
            for (size_t j = 0; j < param.elemCount; j++)
            {
                size_t idx = i * param.elemCount + j;
                T expected = (T)(i + j);
                if (param.recvBuf[idx] != expected)
                {
                    sprintf(this->errMessage, "[%zu] got recvBuf[%zu] = %f, but expected = %f\n",
                                              param.processIdx, idx, (double)param.recvBuf[idx], (double)expected);
                    return TEST_FAILURE;
                }

                if ((param.GetPlaceType() == PT_IN) && (param.sendBuf[idx] != expected))
                {
                    sprintf(this->errMessage, "[%zu] got sendBuf[%zu] = %f, but expected = %f\n",
                                              param.processIdx, idx, (double)param.sendBuf[idx], (double)expected);
                    return TEST_FAILURE;
                }
            }
        }

        if (param.GetPlaceType() == PT_OOP)
        {
            for (size_t i = 0; i < param.processCount * param.elemCount; i++)
            {
                T expected = (param.processIdx + i);
                if (param.sendBuf[i] != expected)
                {
                    sprintf(this->errMessage, "[%zu] got sendBuf[%zu] = %f, but expected = %f\n",
                                              param.processIdx, i, (double)param.sendBuf[i], (double)expected);
                    return TEST_FAILURE;
                }
            }
        }

        return TEST_SUCCESS;
    }

    int Run(TypedTestParam<T>& param)
    {
        for (size_t i = 0; i < param.processCount * param.elemCount; i++)
        {
            param.sendBuf[i] = param.processIdx + i;
            if (param.GetPlaceType() != PT_IN) param.recvBuf[i] = (T)SOME_VALUE;
        }

        /* in case of in-place i-th process already has result in i-th block of send buffer */
        if (param.GetPlaceType() == PT_IN)
        {
            for (size_t i = 0; i < param.elemCount; i++)
            {
                size_t idx = param.processIdx * param.elemCount + i;
                param.sendBuf[idx] = param.processIdx + i;
            }
        }

        CommReq* req = param.dist->AllGather(param.sendBuf, param.elemCount, param.recvBuf,
                                             (DataType)param.GetDataType(), (GroupType)param.GetGroupType());
        param.CompleteRequest(req);
        int result = Check(param);
        return result;
    }
};

RUN_METHOD_DEFINITION(AllGatherTest);
TEST_CASES_DEFINITION(AllGatherTest);
MAIN_FUNCTION();