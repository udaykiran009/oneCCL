#include "base.hpp"

class ParameterSetTest : public BaseTest
{};

TEST_F(ParameterSetTest, test_GetGlobalKernelCount)
{
    ITERATE_OVER_NETS_OPS_TRS(
        EXPECT_EQ(tr.kernelCount, param->GetGlobalKernelCount());
    );

    ITERATE_OVER_DEF_DISTS(
        opParam.paramCount = 1;
        opParam.kernelCount = 800;
        Operation* op = CreateDefaultOperation(opParam);
        EXPECT_EQ(opParam.kernelCount, op->GetParameterSet(0)->GetGlobalKernelCount());
    );
}

TEST_F(ParameterSetTest, test_GetGlobalKernelOffset)
{
    ITERATE_OVER_NETS_OPS_TRS(
        EXPECT_EQ(tr.kernelCount / dist.modelProcessCount * dist.modelProcessIdx, param->GetGlobalKernelOffset());
    );

    ITERATE_OVER_DEF_DISTS(
        opParam.paramCount = 1;
        opParam.kernelCount = 800;
        Operation* op = CreateDefaultOperation(opParam);
        EXPECT_EQ(opParam.kernelCount / dist->GetProcessCount(GT_MODEL) * dist->GetProcessIdx(GT_MODEL),
                  op->GetParameterSet(0)->GetGlobalKernelOffset());
    );
}

TEST_F(ParameterSetTest, test_GetLocalKernelCount)
{
    ITERATE_OVER_NETS_OPS_TRS(
        if (param->IsDistributedUpdate())
            EXPECT_EQ(param->GetOwnedKernelCount() * dist.dataProcessCount, param->GetLocalKernelCount());
        else
            EXPECT_EQ(tr.kernelCount / dist.modelProcessCount, param->GetLocalKernelCount());
    );

    ITERATE_OVER_DEF_DISTS(
        opParam.paramCount = 1;
        opParam.kernelCount = 800;
        Operation* op = CreateDefaultOperation(opParam);
        ParameterSet* param = op->GetParameterSet(0);
        if (param->IsDistributedUpdate())
            EXPECT_EQ(param->GetOwnedKernelCount() * dist->GetProcessCount(GT_DATA),
                      param->GetLocalKernelCount());
        else
            EXPECT_EQ(opParam.kernelCount / dist->GetProcessCount(GT_MODEL), param->GetLocalKernelCount());
    );
}

TEST_F(ParameterSetTest, test_GetOwnedKernelCount)
{
    ITERATE_OVER_NETS_OPS_TRS(
        if (param->IsDistributedUpdate())
            EXPECT_EQ((tr.kernelCount / dist.modelProcessCount + dist.dataProcessCount - 1) / dist.dataProcessCount,
                     param->GetOwnedKernelCount());
        else
            EXPECT_EQ(tr.kernelCount / dist.modelProcessCount, param->GetOwnedKernelCount());
    );

    ITERATE_OVER_DEF_DISTS(
        opParam.paramCount = 1;
        opParam.kernelCount = 800;
        Operation* op = CreateDefaultOperation(opParam);
        ParameterSet* param = op->GetParameterSet(0);
        if (param->IsDistributedUpdate())
            EXPECT_EQ((opParam.kernelCount / dist->GetProcessCount(GT_MODEL) + dist->GetProcessCount(GT_DATA) - 1) / dist->GetProcessCount(GT_DATA),
                     param->GetOwnedKernelCount());
        else
            EXPECT_EQ(opParam.kernelCount / dist->GetProcessCount(GT_MODEL), param->GetOwnedKernelCount());
    );
}

TEST_F(ParameterSetTest, test_GetOwnedKernelOffset)
{
    ITERATE_OVER_NETS_OPS_TRS(
        if (param->IsDistributedUpdate())
            EXPECT_EQ(param->GetOwnedKernelCount() * dist.dataProcessIdx,
                     param->GetOwnedKernelOffset());
        else
            EXPECT_EQ(0, param->GetOwnedKernelOffset());
    );

    ITERATE_OVER_DEF_DISTS(
        opParam.paramCount = 1;
        opParam.kernelCount = 800;
        Operation* op = CreateDefaultOperation(opParam);
        ParameterSet* param = op->GetParameterSet(0);
        if (param->IsDistributedUpdate())
            EXPECT_EQ(param->GetOwnedKernelCount() * dist->GetProcessIdx(GT_DATA),
                     param->GetOwnedKernelOffset());
        else
            EXPECT_EQ(0, param->GetOwnedKernelOffset());
    );
}

TEST_F(ParameterSetTest, test_GetDataType)
{
    ITERATE_OVER_NETS_OPS_TRS(
        EXPECT_EQ(ICCL_DTYPE, param->GetDataType());
    );

    ITERATE_OVER_DEF_DISTS(
        opParam.paramCount = 1;
        opParam.dataType = DataType::DT_FLOAT;
        Operation* op = CreateDefaultOperation(opParam);
        EXPECT_EQ(opParam.dataType, op->GetParameterSet(0)->GetDataType());

        opParam.dataType = DataType::DT_DOUBLE;
        op = CreateDefaultOperation(opParam);
        EXPECT_EQ(opParam.dataType, op->GetParameterSet(0)->GetDataType());
    );
}

TEST_F(ParameterSetTest, test_GetKernelSize)
{
    ITERATE_OVER_NETS_OPS_TRS(
        EXPECT_EQ(tr.kernelSize, param->GetKernelSize());
    );

    ITERATE_OVER_DEF_DISTS(
        opParam.paramCount = 1;
        opParam.kernelSize = 9;
        Operation* op = CreateDefaultOperation(opParam);
        EXPECT_EQ(opParam.kernelSize, op->GetParameterSet(0)->GetKernelSize());

        opParam.kernelSize = 16;
        op = CreateDefaultOperation(opParam);
        EXPECT_EQ(opParam.kernelSize, op->GetParameterSet(0)->GetKernelSize());
    );
}

TEST_F(ParameterSetTest, test_StartWaitGradientComm)
{
    ITERATE_OVER_NETS_OPS_TRS(
        size_t paramLen = param->GetLocalKernelCount() * param->GetKernelSize();
        size_t paramSize = paramLen * DTYPE_SIZE;
        DTYPE* buffer = (DTYPE*)Environment::GetEnv().Alloc(paramSize, CACHELINE_SIZE);
        DTYPE* retBuffer = NULL;
        for (size_t idx = 0; idx < paramLen; idx++) buffer[idx] = 1;
        ASSERT_NO_THROW(param->StartGradientComm(buffer); retBuffer = (DTYPE*)param->WaitGradientComm(););
        if (dist.dataProcessCount > 1) EXPECT_NE((void*)NULL, (void*)retBuffer);
        if (retBuffer) EXPECT_EQ((DTYPE)(1.0 * dist.dataProcessCount), retBuffer[0]);
        Environment::GetEnv().Free(buffer);
    );
}

TEST_F(ParameterSetTest, test_StartWaitIncrementComm)
{
    ITERATE_OVER_NETS_OPS_TRS(
        size_t paramLen = param->GetLocalKernelCount() * param->GetKernelSize();
        size_t paramSize = paramLen * DTYPE_SIZE;
        DTYPE* buffer = (DTYPE*)Environment::GetEnv().Alloc(paramSize, CACHELINE_SIZE);
        DTYPE* retBuffer = NULL;
        for (size_t idx = 0; idx < paramLen; idx++) buffer[idx] = 1;
        ASSERT_NO_THROW(param->StartIncrementComm(buffer); retBuffer = (DTYPE*)param->WaitIncrementComm(););
        if (param->IsDistributedUpdate() && dist.dataProcessCount > 1) EXPECT_NE((void*)NULL, (void*)retBuffer);
        if (retBuffer) EXPECT_EQ((DTYPE)(1.0), retBuffer[0]);
        Environment::GetEnv().Free(buffer);
    );
}

MAIN_FUNCTION();
