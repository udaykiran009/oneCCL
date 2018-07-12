#include "base.hpp"

template <typename T>
class GatherTest : public BaseTest<T>
{
public:
    int Check(TypedTestParam<T>& param)
    {
        CHECK_SELF_COMM(param);

        if (param.processIdx == ROOT_PROCESS_IDX)
        {
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
                }
            }
        }
        else
        {
            for (size_t i = 0; i < param.processCount * param.elemCount; i++)
            {
                T expected = (param.GetPlaceType() != PT_IN) ? (T)SOME_VALUE : (param.processIdx + i);
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
        for (size_t i = 0; i < param.processCount * param.elemCount; i++)
        {
            param.sendBuf[i] = param.processIdx + i;
            if (param.GetPlaceType() != PT_IN) param.recvBuf[i] = (T)SOME_VALUE;
        }

        if (param.GetPlaceType() == PT_OOP || (param.GetPlaceType() == PT_IN && param.GetSizeType() != ST_SMALL))
        {
            CommReq* req = param.dist->Gather(param.sendBuf, param.elemCount, param.recvBuf,
                                              (DataType)param.GetDataType(), ROOT_PROCESS_IDX, (GroupType)param.GetGroupType());
            param.CompleteRequest(req);
            int result = Check(param);
            return result;
        }
        else
        {
            /* FIXME: fix (PT_IN && ST_SMALL) case */
            return TEST_SUCCESS;
        }
    }
};

RUN_METHOD_DEFINITION(GatherTest);
TEST_CASES_DEFINITION(GatherTest);
MAIN_FUNCTION();