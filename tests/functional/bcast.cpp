#include "base.hpp"
template <typename T>
class BcastTest : public BaseTest<T>
{
public:
    int Check(TypedTestParam<T>& param)
    {
        for (size_t i = 0; i < param.elemCount; i++)
        {
            if (param.sendBuf[i] != (T)i)
            {
                sprintf(this->errMessage, "[%zu] got sendBuf[%zu] = %f, but expected = %f\n",
                                          param.processIdx, i, (double)param.sendBuf[i], (double)i);
                return TEST_FAILURE;
            }
        }
        return TEST_SUCCESS;
    }

    int Run(TypedTestParam<T>& param)
    {
        for (size_t i = 0; i < param.elemCount; i++)
        {
            if (param.processIdx == ROOT_PROCESS_IDX) param.sendBuf[i] = i;
            else param.sendBuf[i] = (T)SOME_VALUE;
        }
        CommReq* req = param.dist->Bcast(param.sendBuf, param.elemCount, (DataType)param.GetDataType(),
                                         ROOT_PROCESS_IDX, (GroupType)param.GetGroupType());
        param.CompleteRequest(req);
        int result = Check(param);
        return result;
    }
};

RUN_METHOD_DEFINITION(BcastTest);
TEST_CASES_DEFINITION(BcastTest);
MAIN_FUNCTION();
