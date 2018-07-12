#include "base.hpp"

template <typename T>
class ReduceScatterTest : public BaseTest<T>
{
public:
    int Check(TypedTestParam<T>& param)
    {
        CHECK_SELF_COMM(param);

        for (size_t i = 0; i < param.elemCount; i++)
        {
            T expected = (param.processCount * param.processIdx + ((param.processCount - 1) * param.processCount) / 2);
            if (param.recvBuf[i] != expected)
            {
                sprintf(this->errMessage, "[%zu] got recvBuf[%zu] = %f, but expected = %f\n",
                                          param.processIdx, i, (double)param.recvBuf[i], (double)expected);
                return TEST_FAILURE;
            }
        }

        for (size_t i = 1; i < param.processCount; i++)
        {
            for (size_t j = 0; j < param.elemCount; j++)
            {
                /* check we didn't corrupt the sendBuf somehow */
                size_t idx = i * param.elemCount + j;
                T expected = (T)SOME_VALUE;
                if (param.recvBuf[idx] != expected)
                {
                    sprintf(this->errMessage, "[%zu] got recvBuf[%zu] = %f, but expected = %f\n",
                                              param.processIdx, idx, (double)param.recvBuf[idx], (double)expected);
                    return TEST_FAILURE;
                }
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
                param.sendBuf[i * param.elemCount + j] = param.processIdx + i;
                if (param.GetPlaceType() != PT_IN) param.recvBuf[i * param.elemCount + j] = (T)SOME_VALUE;
            }
        }

        if (param.GetPlaceType() == PT_OOP)
        {
            CommReq* req = param.dist->ReduceScatter(param.sendBuf, param.recvBuf, param.elemCount, (DataType)param.GetDataType(),
                                                     RT_SUM, (GroupType)param.GetGroupType());
            param.CompleteRequest(req);
            int result = Check(param);
            return result;
        }
        else
        {
            /* FIXME: fix PT_IN case */
            return TEST_SUCCESS;
        }
    }
};

RUN_METHOD_DEFINITION(ReduceScatterTest);
TEST_CASES_DEFINITION(ReduceScatterTest);
MAIN_FUNCTION();
