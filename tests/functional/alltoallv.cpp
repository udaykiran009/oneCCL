#include "base.hpp"

template <typename T>
class AlltoAllvTest : public BaseTest<T>
{
public:
    size_t totalCount;
    int Check(TypedTestParam<T>& param)
    {
        CHECK_SELF_COMM(param);
        for (size_t idx = 0; idx < totalCount; idx++)
        {
            T expected = (T)(param.processIdx);

            if (param.recvBuf[idx] != expected)
            {
                printf("[%zu] got recvBuf[%zu] = %f, but expected = %f\n", param.processIdx, idx, (double)param.recvBuf[idx], (double)expected);
                return TEST_FAILURE;
            }
        }
        return TEST_SUCCESS;
    }

    int Run(TypedTestParam<T>& param)
    {
        size_t* recvCounts  = (size_t*)Environment::GetEnv().Alloc(param.processCount*sizeof(size_t), 64);
        size_t* sendCounts  = (size_t*)Environment::GetEnv().Alloc(param.processCount*sizeof(size_t), 64);
        size_t* recvOffsets = (size_t*)Environment::GetEnv().Alloc(param.processCount*sizeof(size_t), 64);
        size_t* sendOffsets = (size_t*)Environment::GetEnv().Alloc(param.processCount*sizeof(size_t), 64);

        size_t sub = 0;
        size_t disp = 0;

        for (size_t i = 0; i < param.processCount; i++)
        {
            recvOffsets[i] = disp;
            sendOffsets[i] = disp;

            sub = i + param.processIdx;
            if (param.elemCount > sub)
            {
               sendCounts[i] = param.elemCount - sub;
               recvCounts[i] = param.elemCount - sub;
            } else {
               sendCounts[i] = param.elemCount;
               recvCounts[i] = param.elemCount;
            }
            for (size_t j = 0; j < sendCounts[i]; j++)
            {
                param.sendBuf[sendOffsets[i] + j] = i;
                if (param.GetPlaceType() != PT_IN) param.recvBuf[recvOffsets[i] + j] = (T)SOME_VALUE;
            }
            disp += sendCounts[i];
        }
        totalCount = disp;

        CommReq* req = param.dist->AlltoAllv(param.sendBuf, sendCounts, sendOffsets, param.recvBuf, recvCounts, recvOffsets,
                                            (DataType)param.GetDataType(), (GroupType)param.GetGroupType());

        param.CompleteRequest(req);
        int result = Check(param);
        return result;
    }
};


RUN_METHOD_DEFINITION(AlltoAllvTest);
TEST_CASES_DEFINITION(AlltoAllvTest);
MAIN_FUNCTION();