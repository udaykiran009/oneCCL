#include "base.hpp"

template <typename T>
class BarrierTest : public BaseTest<T>
{
public:
    int Check(TypedTestParam<T>& param UNUSED_ATTR)
    {
        return TEST_SUCCESS;
    }

    int Run(TypedTestParam<T>& param)
    {
        try
        {
            param.dist->Barrier((GroupType)param.GetGroupType());
            int result = Check(param);
            return result;
        }
        catch(...)
        {
            return TEST_FAILURE;
        }
    }
};


RUN_METHOD_DEFINITION(BarrierTest);
TEST_CASES_DEFINITION(BarrierTest);
MAIN_FUNCTION();