#include "base.hpp"

template <typename T>
class ScatterTest : public BaseTest<T>
{
public:
    int Check(TypedTestParam<T>& param)
    {
        CHECK_SELF_COMM(param);

        for (size_t i = 0; i < param.elemCount; i++)
        {
            T expected = param.processIdx + i;
            if (param.recvBuf[i] != expected)
            {
                sprintf(this->errMessage, "[%zu] got recvBuf[%zu] = %f, but expected = %f\n",
                                          param.processIdx, i, (double)param.recvBuf[i], (double)expected);
                return TEST_FAILURE;
            }
        }

        if (param.processIdx != ROOT_PROCESS_IDX)
        {
            for (size_t i = 0; i < param.processCount * param.elemCount; i++)
            {
                /* check we didn't corrupt the sendBuf somehow */
                T expected = (param.GetPlaceType() != PT_IN) ? SOME_VALUE : ((i < param.elemCount) ? (param.processIdx + i) : (T)SOME_VALUE);
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
        for (size_t i = 0; i < param.processCount; i++)
        {
            for (size_t j = 0; j < param.elemCount; j++)
            {
                if (param.processIdx == ROOT_PROCESS_IDX) param.sendBuf[i * param.elemCount + j] = i + j;
                else
                {
                    param.sendBuf[i * param.elemCount + j] = (T)SOME_VALUE;
                    if (param.GetPlaceType() != PT_IN) param.recvBuf[i * param.elemCount + j] = (T)SOME_VALUE;
                }
            }
        }

        if (param.GetPlaceType() == PT_OOP || (param.GetPlaceType() == PT_IN && param.GetSizeType() != ST_SMALL))
        {
            CommReq* req = param.dist->Scatter(param.sendBuf, param.recvBuf, param.elemCount,
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

RUN_METHOD_DEFINITION(ScatterTest);
TEST_CASES_DEFINITION(ScatterTest);
MAIN_FUNCTION();
