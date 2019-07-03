#include "base.hpp"

class OperationRegInfoTest : public BaseTest
{};

TEST_F(OperationRegInfoTest, test_CreateOperationRegInfo)
{
    ITERATE_OVER_DEF_DISTS_OPTYPES(
        OperationRegInfo* regInfo = CreateDefaultOperationRegInfo(opParam);
        EXPECT_NE((void*)NULL, (void*)regInfo);
        DeleteDefaultOperationRegInfo(regInfo);
    );
}

TEST_F(OperationRegInfoTest, test_AddInput)
{
    ITERATE_OVER_DEF_DISTS_OPTYPES(
        OperationRegInfo* regInfo = CreateDefaultOperationRegInfo(opParam);
        EXPECT_EQ(DEFAULT_IN_ACT_COUNT + 0, regInfo->AddInput(1, 1, DataType::DT_FLOAT));
        EXPECT_EQ(DEFAULT_IN_ACT_COUNT + 1, regInfo->AddInput(1, 1, DataType::DT_DOUBLE));
        EXPECT_EQ(DEFAULT_IN_ACT_COUNT + 2, regInfo->AddInput(100, 100, DataType::DT_DOUBLE));
        DeleteDefaultOperationRegInfo(regInfo);
    );

    ITERATE_OVER_DEF_DISTS_OPTYPES(
        OperationRegInfo* regInfo = CreateDefaultOperationRegInfo(opParam);
        ASSERT_EXIT(regInfo->AddInput(0, 1, DataType::DT_DOUBLE),  ::testing::ExitedWithCode(1), COUNT_AND_SIZE_SHOULD_BE_POSITIVE);
        DeleteDefaultOperationRegInfo(regInfo);
    );

    ITERATE_OVER_DEF_DISTS_OPTYPES(
        OperationRegInfo* regInfo = CreateDefaultOperationRegInfo(opParam);
        ASSERT_EXIT(regInfo->AddInput(1, 0, DataType::DT_DOUBLE),  ::testing::ExitedWithCode(1), COUNT_AND_SIZE_SHOULD_BE_POSITIVE);
        DeleteDefaultOperationRegInfo(regInfo);
    );

    ITERATE_OVER_DEF_DISTS_OPTYPES(
        OperationRegInfo* regInfo = CreateDefaultOperationRegInfo(opParam);
        ASSERT_EXIT(regInfo->AddInput(size_t(-1), 1, DataType::DT_DOUBLE), ::testing::ExitedWithCode(1), COUNT_AND_SIZE_SHOULD_BE_POSITIVE);
        DeleteDefaultOperationRegInfo(regInfo);
    );

    ITERATE_OVER_DEF_DISTS_OPTYPES(
        OperationRegInfo* regInfo = CreateDefaultOperationRegInfo(opParam);
        ASSERT_EXIT(regInfo->AddInput(1, size_t(-1), DataType::DT_DOUBLE), ::testing::ExitedWithCode(1), COUNT_AND_SIZE_SHOULD_BE_POSITIVE);
        DeleteDefaultOperationRegInfo(regInfo);
    );
}

TEST_F(OperationRegInfoTest, test_AddOutput)
{
    ITERATE_OVER_DEF_DISTS_OPTYPES(
        OperationRegInfo* regInfo = CreateDefaultOperationRegInfo(opParam);
        EXPECT_EQ(DEFAULT_OUT_ACT_COUNT + 0, regInfo->AddOutput(1, 1, DataType::DT_FLOAT));
        EXPECT_EQ(DEFAULT_OUT_ACT_COUNT + 1, regInfo->AddOutput(1, 1, DataType::DT_DOUBLE));
        EXPECT_EQ(DEFAULT_OUT_ACT_COUNT + 2, regInfo->AddOutput(100, 100, DataType::DT_DOUBLE));
        DeleteDefaultOperationRegInfo(regInfo);
    );

    ITERATE_OVER_DEF_DISTS_OPTYPES(
        OperationRegInfo* regInfo = CreateDefaultOperationRegInfo(opParam);
        ASSERT_EXIT(regInfo->AddOutput(0, 1, DataType::DT_DOUBLE),  ::testing::ExitedWithCode(1), COUNT_AND_SIZE_SHOULD_BE_POSITIVE);
        DeleteDefaultOperationRegInfo(regInfo);
    );

    ITERATE_OVER_DEF_DISTS_OPTYPES(
        OperationRegInfo* regInfo = CreateDefaultOperationRegInfo(opParam);
        ASSERT_EXIT(regInfo->AddOutput(1, 0, DataType::DT_DOUBLE),  ::testing::ExitedWithCode(1), COUNT_AND_SIZE_SHOULD_BE_POSITIVE);
        DeleteDefaultOperationRegInfo(regInfo);
    );

    ITERATE_OVER_DEF_DISTS_OPTYPES(
        OperationRegInfo* regInfo = CreateDefaultOperationRegInfo(opParam);
        ASSERT_EXIT(regInfo->AddOutput(size_t(-1), 1, DataType::DT_DOUBLE), ::testing::ExitedWithCode(1), COUNT_AND_SIZE_SHOULD_BE_POSITIVE);
        DeleteDefaultOperationRegInfo(regInfo);
    );

    ITERATE_OVER_DEF_DISTS_OPTYPES(
        OperationRegInfo* regInfo = CreateDefaultOperationRegInfo(opParam);
        ASSERT_EXIT(regInfo->AddOutput(1, size_t(-1), DataType::DT_DOUBLE), ::testing::ExitedWithCode(1), COUNT_AND_SIZE_SHOULD_BE_POSITIVE);
        DeleteDefaultOperationRegInfo(regInfo);
    );
}

TEST_F(OperationRegInfoTest, test_AddParameterSet)
{
    ITERATE_OVER_DEF_DISTS_OPTYPES(
        OperationRegInfo* regInfo = CreateDefaultOperationRegInfo(opParam);
        EXPECT_EQ(DEFAULT_PARAM_COUNT + 0, regInfo->AddParameterSet(1, 1, DataType::DT_FLOAT));
        EXPECT_EQ(DEFAULT_PARAM_COUNT + 1, regInfo->AddParameterSet(1, 1, DataType::DT_DOUBLE));
        EXPECT_EQ(DEFAULT_PARAM_COUNT + 2, regInfo->AddParameterSet(100, 100, DataType::DT_DOUBLE));
        DeleteDefaultOperationRegInfo(regInfo);
    );

    ITERATE_OVER_DEF_DISTS_OPTYPES(
        OperationRegInfo* regInfo = CreateDefaultOperationRegInfo(opParam);
        ASSERT_EXIT(regInfo->AddParameterSet(0, 1, DataType::DT_DOUBLE),  ::testing::ExitedWithCode(1), COUNT_AND_SIZE_SHOULD_BE_POSITIVE);
        DeleteDefaultOperationRegInfo(regInfo);
    );

    ITERATE_OVER_DEF_DISTS_OPTYPES(
        OperationRegInfo* regInfo = CreateDefaultOperationRegInfo(opParam);
        ASSERT_EXIT(regInfo->AddParameterSet(1, 0, DataType::DT_DOUBLE),  ::testing::ExitedWithCode(1), COUNT_AND_SIZE_SHOULD_BE_POSITIVE);
        DeleteDefaultOperationRegInfo(regInfo);
    );

    ITERATE_OVER_DEF_DISTS_OPTYPES(
        OperationRegInfo* regInfo = CreateDefaultOperationRegInfo(opParam);
        ASSERT_EXIT(regInfo->AddParameterSet(size_t(-1), 1, DataType::DT_DOUBLE), ::testing::ExitedWithCode(1), COUNT_AND_SIZE_SHOULD_BE_POSITIVE);
        DeleteDefaultOperationRegInfo(regInfo);
    );

    ITERATE_OVER_DEF_DISTS_OPTYPES(
        OperationRegInfo* regInfo = CreateDefaultOperationRegInfo(opParam);
        ASSERT_EXIT(regInfo->AddParameterSet(1, size_t(-1), DataType::DT_DOUBLE), ::testing::ExitedWithCode(1), COUNT_AND_SIZE_SHOULD_BE_POSITIVE);
        DeleteDefaultOperationRegInfo(regInfo);
    );
}

TEST_F(OperationRegInfoTest, test_AddParameterSet_without_set_quant)
{
    Session* session = Environment::GetEnv().CreateSession();
    OperationRegInfo* regInfo = session->CreateOperationRegInfo(DEFAULT_OP_TYPE);
    ASSERT_EXIT(regInfo->AddParameterSet(1, 1, DataType::DT_DOUBLE, false, CT_QUANTIZATION), ::testing::ExitedWithCode(1), SET_QUANT_PARAMS);
    DeleteDefaultOperationRegInfo(regInfo);
    Environment::GetEnv().DeleteSession(session);
}

TEST_F(OperationRegInfoTest, test_GetQuantizationParams_before_set)
{
    ASSERT_EQ((void*)NULL, Environment::GetEnv().GetQuantizationParams());
}

TEST_F(OperationRegInfoTest, test_SetQuantizationParams)
{
    ASSERT_NO_THROW(SetDefaultQuantParams());
}

TEST_F(OperationRegInfoTest, test_GetQuantizationParams)
{
    EXPECT_NE((void*)NULL, Environment::GetEnv().GetQuantizationParams());
}

TEST_F(OperationRegInfoTest, test_SetQuantizationParams_twice)
{
    if (!(Environment::GetEnv().GetQuantizationParams()))
        SetDefaultQuantParams();
    ASSERT_EXIT(SetDefaultQuantParams(), ::testing::ExitedWithCode(1), QUANT_PARAMS_CAN_BE_SET_ONLY_ONCE);
}

TEST_F(OperationRegInfoTest, test_AddParameterSet_with_set_quant)
{
    Session* session = Environment::GetEnv().CreateSession();
    OperationRegInfo* regInfo = session->CreateOperationRegInfo(DEFAULT_OP_TYPE);
    if (!(Environment::GetEnv().GetQuantizationParams()))
        SetDefaultQuantParams();
    ASSERT_NO_THROW(regInfo->AddParameterSet(1, 1, DataType::DT_DOUBLE, false, CT_QUANTIZATION));
    DeleteDefaultOperationRegInfo(regInfo);
    Environment::GetEnv().DeleteSession(session);
}

TEST_F(OperationRegInfoTest, test_Validate)
{
    ITERATE_OVER_DEF_DISTS_OPTYPES(
        OperationRegInfo* regInfo = CreateDefaultOperationRegInfo(opParam);
        for (size_t idx = 0; idx < TRIPLET_COUNT; idx++)
        {
            EXPECT_EQ(DEFAULT_IN_ACT_COUNT + idx, regInfo->AddInput(10, 10, ICCL_DTYPE));
            EXPECT_EQ(DEFAULT_OUT_ACT_COUNT + idx, regInfo->AddOutput(10, 10, ICCL_DTYPE));
            EXPECT_EQ(DEFAULT_PARAM_COUNT + idx, regInfo->AddParameterSet(10, 10, ICCL_DTYPE));
        }
        if (IS_OP_SUPPORTED(opType))
            ASSERT_NO_THROW(regInfo->Validate());
        else
            ASSERT_EXIT(regInfo->Validate(), ::testing::ExitedWithCode(1), IS_NOT_SUPPORTED);
        DeleteDefaultOperationRegInfo(regInfo);
    );
}

TEST_F(OperationRegInfoTest, test_SetGetName)
{
    ITERATE_OVER_DEF_DISTS_OPTYPES(
        opParam.opName = "test_op_name";
        Operation* op = CreateDefaultOperation(opParam);
        EXPECT_EQ(0, strcmp(opParam.opName, op->GetName()));
    );
}

TEST_F(OperationRegInfoTest, test_SetNameNull)
{
    ITERATE_OVER_DEF_DISTS_OPTYPES(
        opParam.opName = NULL;
        ASSERT_EXIT(CreateDefaultOperationRegInfo(opParam), ::testing::ExitedWithCode(1), NAME_IS_NULL);
    );
}

TEST_F(OperationRegInfoTest, test_GetParameterSetCountAfterDeleteOperationRegInfo)
{
    ITERATE_OVER_DEF_DISTS_OPTYPES(
        opParam.paramCount = 0;
        OperationRegInfo* regInfo = CreateDefaultOperationRegInfo(opParam);
        DeleteDefaultOperationRegInfo(regInfo);

        opParam.paramCount = 1;
        Operation* op = CreateDefaultOperation(opParam);
        EXPECT_EQ(opParam.paramCount, op->GetParameterSetCount());
    );
}

MAIN_FUNCTION();
