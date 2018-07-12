#include "base.hpp"

template <typename T>
class AllGathervTest : public BaseTest<T>
{
public:
    size_t* recvCounts;
    size_t* offset;
    int Check(TypedTestParam<T>& param)
    {
        CHECK_SELF_COMM(param);

        for (size_t i = 0; i < param.processCount; i++)
        {
            for (size_t j = 0; j < recvCounts[i]; j++)
            {
                size_t idx = offset[i] + j;
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
            for (size_t i = 0; i < param.processCount; i++)
            {
                for (size_t j = 0; j < recvCounts[i]; j++)
                {
                    T expected = (param.processIdx + i + j);
                    if (param.sendBuf[offset[i] + j] != expected)
                    {
                        sprintf(this->errMessage, "[%zu] got sendBuf[%zu] = %f, but expected = %f\n",
                                              param.processIdx, i, (double)param.sendBuf[i], (double)expected);
                        return TEST_FAILURE;
                    }
                }
            }
        }

        return TEST_SUCCESS;
    }

    int Run(TypedTestParam<T>& param)
    {
        recvCounts = (size_t*)Environment::GetEnv().Alloc(param.processCount*sizeof(size_t), 64);
        offset = (size_t*)Environment::GetEnv().Alloc(param.processCount*sizeof(size_t), 64);

        offset[0] = 0;
        recvCounts[0] = param.elemCount;

        for (size_t i = 1; i < param.processCount; i++)
        {
            if (param.elemCount > i)
               recvCounts[i] = param.elemCount - i;
            else
               recvCounts[i] = param.elemCount;

            offset[i] = recvCounts[i - 1] + offset[i - 1];
        }

        for (size_t i = 0; i < param.processCount; i++)
        {
            for (size_t j = 0; j < recvCounts[i]; j++)
            {
              param.sendBuf[offset[i] + j] = param.processIdx + i + j;
              if (param.GetPlaceType() != PT_IN) param.recvBuf[offset[i] + j] = (T)SOME_VALUE;
            }
        }

        /* in case of in-place i-th process already has result in i-th block of send buffer */
        if (param.GetPlaceType() == PT_IN)
        {
            for (size_t i = 0; i < recvCounts[param.processIdx]; i++)
            {
                param.sendBuf[offset[param.processIdx] + i] = param.processIdx + i;
            }
        }

        CommReq* req = param.dist->AllGatherv(param.sendBuf, recvCounts[param.processIdx], param.recvBuf, recvCounts,
                                             (DataType)param.GetDataType(), (GroupType)param.GetGroupType());
        param.CompleteRequest(req);
        int result = Check(param);
        Environment::GetEnv().Free(recvCounts);
        return result;
    }
};

RUN_METHOD_DEFINITION(AllGathervTest);
TEST_CASES_DEFINITION(AllGathervTest);
MAIN_FUNCTION();