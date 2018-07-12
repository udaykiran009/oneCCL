#include "base.hpp"

class OperationTest : public BaseTest
{};

TEST_F(OperationTest, test_CreatOperationWithSession)
{
    /* normal case */
    ITERATE_OVER_DEF_DISTS_OPTYPES(
            OperationRegInfo* regInfo = CreateDefaultOperationRegInfo(opParam);
            ASSERT_NO_THROW(defSession->AddOperation(regInfo, dist));
            DeleteDefaultOperationRegInfo(regInfo);
        );

    /* erroneous case (wo setting of batch size) */
    ITERATE_OVER_DEF_DISTS_OPTYPES(
            Session* session = Environment::GetEnv().CreateSession();
            OperationRegInfo* regInfo = session->CreateOperationRegInfo(DEFAULT_OP_TYPE);
            ASSERT_EXIT(session->AddOperation(regInfo, dist), ::testing::ExitedWithCode(1), BATCH_SIZE_SHOULD_BE_SET_BEFORE);
            DeleteDefaultOperationRegInfo(regInfo);
            Environment::GetEnv().DeleteSession(session);
        );
}

TEST_F(OperationTest, test_CreatOperationWithNullParams)
{
    ITERATE_OVER_DEF_DISTS_OPTYPES(
            OperationRegInfo* regInfo = CreateDefaultOperationRegInfo(opParam);
            ASSERT_EXIT(defSession->AddOperation(NULL, dist), ::testing::ExitedWithCode(1), SESSION_OR_REGINFO_IS_NULL);
            DeleteDefaultOperationRegInfo(regInfo);
        );

    ITERATE_OVER_DEF_DISTS_OPTYPES(
            OperationRegInfo* regInfo = CreateDefaultOperationRegInfo(opParam);
            ASSERT_EXIT(defSession->AddOperation(NULL, NULL), ::testing::ExitedWithCode(1), SESSION_OR_REGINFO_IS_NULL);
            DeleteDefaultOperationRegInfo(regInfo);
        );
}

TEST_F(OperationTest, test_SetDoubleDistribution)
{
    ITERATE_OVER_DEF_DISTS_OPTYPES(
            Operation* op = CreateDefaultOperation(opParam);
            ASSERT_EXIT(op->SetDistribution(dist), ::testing::ExitedWithCode(1), DIST_CAN_BE_SET_ONLY_ONCE);
        );
}

TEST_F(OperationTest, test_SetNullDistribution)
{
    ITERATE_OVER_DEF_DISTS_OPTYPES(
            Operation* op = CreateOperationWithoutDistribution(opParam);
            ASSERT_EXIT(op->SetDistribution(NULL), ::testing::ExitedWithCode(1), DISTRIBUTION_IS_NULL);
        );
}

TEST_F(OperationTest, test_SetDistribution)
{
    ITERATE_OVER_DEF_DISTS_OPTYPES(
            Operation* op = CreateOperationWithoutDistribution(opParam);
            ASSERT_NO_THROW(op->SetDistribution(dist));
        );

    ITERATE_OVER_DEF_DISTS_OPTYPES(
            Operation* op = CreateOperationWithoutDistribution(opParam);
            op->SetDistribution(dist);
            ASSERT_EQ(op->GetDistribution(), dist);
        );
}

TEST_F(OperationTest, test_SetPrevNull)
{
    ITERATE_OVER_DEF_DISTS(
        Operation* op = CreateDefaultOperation(opParam);
        ASSERT_NO_THROW(op->SetPrev(NULL, 0, 0));
    );
}

TEST_F(OperationTest, test_SetPrevNextInvalidIdx)
{
    /* negative idx */
    ITERATE_OVER_DEF_DISTS(
        Operation* op1 = CreateDefaultOperation(opParam);
        Operation* op2 = CreateDefaultOperation(opParam);
        ASSERT_EXIT(op1->SetPrev(op2, size_t(-1), 0), ::testing::ExitedWithCode(1), INVALID_INPUT_ACT_IDX);
    );

    ITERATE_OVER_DEF_DISTS(
        Operation* op1 = CreateDefaultOperation(opParam);
        Operation* op2 = CreateDefaultOperation(opParam);
        ASSERT_EXIT(op1->SetNext(op2, size_t(-1), 0), ::testing::ExitedWithCode(1), INVALID_OUTPUT_ACT_IDX);
    );

    /* out of range idx */
    ITERATE_OVER_DEF_DISTS(
        Operation* op1 = CreateDefaultOperation(opParam);
        Operation* op2 = CreateDefaultOperation(opParam);
        ASSERT_EXIT(op1->SetPrev(op2, 0, DEFAULT_OUT_ACT_COUNT), ::testing::ExitedWithCode(1), INVALID_OUTPUT_ACT_IDX);
    );

    ITERATE_OVER_DEF_DISTS(
        Operation* op1 = CreateDefaultOperation(opParam);
        Operation* op2 = CreateDefaultOperation(opParam);
        ASSERT_EXIT(op1->SetPrev(op2, DEFAULT_IN_ACT_COUNT, 0), ::testing::ExitedWithCode(1), INVALID_INPUT_ACT_IDX);
    );

    ITERATE_OVER_DEF_DISTS(
        Operation* op1 = CreateDefaultOperation(opParam);
        Operation* op2 = CreateDefaultOperation(opParam);
        ASSERT_EXIT(op1->SetNext(op2, 0, DEFAULT_IN_ACT_COUNT), ::testing::ExitedWithCode(1), INVALID_INPUT_ACT_IDX);
    );

    ITERATE_OVER_DEF_DISTS(
        Operation* op1 = CreateDefaultOperation(opParam);
        Operation* op2 = CreateDefaultOperation(opParam);
        ASSERT_EXIT(op1->SetNext(op2, DEFAULT_OUT_ACT_COUNT, 0), ::testing::ExitedWithCode(1), INVALID_OUTPUT_ACT_IDX);
    );

    /* ops wo any activations */
    ITERATE_OVER_DEF_DISTS(
        opParam.inActCount = 0; opParam.outActCount = 0;
        Operation* op1 = CreateDefaultOperation(opParam);
        Operation* op2 = CreateDefaultOperation(opParam);
        ASSERT_EXIT(op1->SetPrev(op2, 0, 0), ::testing::ExitedWithCode(1), INVALID_INPUT_ACT_IDX);
    );

    ITERATE_OVER_DEF_DISTS(
        opParam.inActCount = 0; opParam.outActCount = 0;
        Operation* op1 = CreateDefaultOperation(opParam);
        Operation* op2 = CreateDefaultOperation(opParam);
        ASSERT_EXIT(op1->SetNext(op2, 0, 0), ::testing::ExitedWithCode(1), INVALID_OUTPUT_ACT_IDX);
    );

    /* ops without corresponding activation on other side */
    ITERATE_OVER_DEF_DISTS(
        opParam.inActCount = 1; opParam.outActCount = 1;
        Operation* op1 = CreateDefaultOperation(opParam);
        opParam.inActCount = 0; opParam.outActCount = 0;
        Operation* op2 = CreateDefaultOperation(opParam);
        ASSERT_EXIT(op1->SetPrev(op2, 0, 0), ::testing::ExitedWithCode(1), INVALID_OUTPUT_ACT_IDX);
    );

    ITERATE_OVER_DEF_DISTS(
        opParam.inActCount = 1; opParam.outActCount = 1;
        Operation* op1 = CreateDefaultOperation(opParam);
        opParam.inActCount = 0; opParam.outActCount = 0;
        Operation* op2 = CreateDefaultOperation(opParam);
        ASSERT_EXIT(op1->SetNext(op2, 0, 0), ::testing::ExitedWithCode(1), INVALID_INPUT_ACT_IDX);
    );
}

TEST_F(OperationTest, test_SetPrevNextCorrect)
{
    ITERATE_OVER_DEF_DISTS(
        opParam.inActCount = 1; opParam.outActCount = 0;
        Operation* op1 = CreateDefaultOperation(opParam);
        opParam.inActCount = 0; opParam.outActCount = 1;
        Operation* op2 = CreateDefaultOperation(opParam);
        ASSERT_NO_THROW(op1->SetPrev(op2, 0, 0));
    );

    ITERATE_OVER_DEF_DISTS(
        opParam.inActCount = 0; opParam.outActCount = 1;
        Operation* op1 = CreateDefaultOperation(opParam);
        opParam.inActCount = 1; opParam.outActCount = 0;
        Operation* op2 = CreateDefaultOperation(opParam);
        ASSERT_NO_THROW(op1->SetNext(op2, 0, 0));
    );
}

TEST_F(OperationTest, test_SetPrevNextDiffentFmCount)
{
    ITERATE_OVER_DEF_DISTS(
        opParam.fmCount = 1;
        Operation* op1 = CreateDefaultOperation(opParam);
        opParam.fmCount = 2;
        Operation* op2 = CreateDefaultOperation(opParam);
        ASSERT_EXIT(op1->SetPrev(op2, 0, 0), ::testing::ExitedWithCode(1), ACT_SIZES_MUST_MATCH);
    );

    ITERATE_OVER_DEF_DISTS(
        opParam.fmCount = 1;
        Operation* op1 = CreateDefaultOperation(opParam);
        opParam.fmCount = 2;
        Operation* op2 = CreateDefaultOperation(opParam);
        ASSERT_EXIT(op1->SetNext(op2, 0, 0), ::testing::ExitedWithCode(1), ACT_SIZES_MUST_MATCH);
    );
}

TEST_F(OperationTest, test_SetPrevNextDiffentDataTypes)
{
    ITERATE_OVER_DEF_DISTS(
        opParam.dataType = DataType::DT_FLOAT;
        Operation* op1 = CreateDefaultOperation(opParam);
        opParam.dataType = DataType::DT_DOUBLE;
        Operation* op2 = CreateDefaultOperation(opParam);
        ASSERT_EXIT(op1->SetPrev(op2, 0, 0), ::testing::ExitedWithCode(1), DATA_TYPES_MUST_MATCH);
    );
}

TEST_F(OperationTest, test_GetGlobalMinibatchSize)
{
    ITERATE_OVER_NETS_OPS(
        ASSERT_EQ(GLOBAL_MINIBATCH_SIZE, mlslOp->GetGlobalMinibatchSize());
    );

    ITERATE_OVER_DEF_DISTS(
        Operation* op = CreateDefaultOperation(opParam);
        ASSERT_EQ(GLOBAL_MINIBATCH_SIZE, op->GetGlobalMinibatchSize());
    );
}


TEST_F(OperationTest, test_GetGlobalMinibatchOffset)
{
    ITERATE_OVER_NETS_OPS(
        ASSERT_EQ(GLOBAL_MINIBATCH_SIZE / dist.dataProcessCount * dist.dataProcessIdx, mlslOp->GetGlobalMinibatchOffset());
    );

    ITERATE_OVER_DEF_DISTS(
        Operation* op = CreateDefaultOperation(opParam);
        ASSERT_EQ(GLOBAL_MINIBATCH_SIZE / dist->GetProcessCount(GT_DATA) * dist->GetProcessIdx(GT_DATA),
                  op->GetGlobalMinibatchOffset());
    );
}

TEST_F(OperationTest, test_GetLocalMinibatchSize)
{
    ITERATE_OVER_NETS_OPS(
        ASSERT_EQ(GLOBAL_MINIBATCH_SIZE / dist.dataProcessCount, mlslOp->GetLocalMinibatchSize());
    );

    ITERATE_OVER_DEF_DISTS(
        Operation* op = CreateDefaultOperation(opParam);
        ASSERT_EQ(GLOBAL_MINIBATCH_SIZE / dist->GetProcessCount(GT_DATA), op->GetLocalMinibatchSize());
    );
}

TEST_F(OperationTest, test_GetInputCount)
{
    ITERATE_OVER_NETS_OPS(
        if (opIdx == 0) ASSERT_EQ(0, mlslOp->GetInputCount());
        else ASSERT_EQ(TRIPLET_COUNT, mlslOp->GetInputCount());
    );

    ITERATE_OVER_DEF_DISTS(
        opParam.inActCount = 10;
        Operation* op = CreateDefaultOperation(opParam);
        ASSERT_EQ(opParam.inActCount, op->GetInputCount());
    );
}

TEST_F(OperationTest, test_GetOutputCount)
{
    ITERATE_OVER_NETS_OPS(
        if (opIdx == (OPERATION_COUNT - 1)) ASSERT_EQ(0, mlslOp->GetOutputCount());
        else ASSERT_EQ(TRIPLET_COUNT, mlslOp->GetOutputCount());
    );

    ITERATE_OVER_DEF_DISTS(
        opParam.outActCount = 10;
        Operation* op = CreateDefaultOperation(opParam);
        ASSERT_EQ(opParam.outActCount, op->GetOutputCount());
    );
}

TEST_F(OperationTest, test_GetParameterSetCount)
{
    ITERATE_OVER_NETS_OPS(
        ASSERT_EQ(TRIPLET_COUNT, mlslOp->GetParameterSetCount());
    );

    ITERATE_OVER_DEF_DISTS(
        opParam.paramCount = 10;
        Operation* op = CreateDefaultOperation(opParam);
        ASSERT_EQ(opParam.paramCount, op->GetParameterSetCount());
    );
}

TEST_F(OperationTest, test_HasParameterSets)
{
    ITERATE_OVER_NETS_OPS(
        ASSERT_EQ((TRIPLET_COUNT > 0) ? true : false, mlslOp->HasParameterSets());
    );

    ITERATE_OVER_DEF_DISTS(
        opParam.paramCount = 0;
        Operation* op = CreateDefaultOperation(opParam);
        ASSERT_EQ(false, op->HasParameterSets());
    );

    ITERATE_OVER_DEF_DISTS(
        opParam.paramCount = 1;
        Operation* op = CreateDefaultOperation(opParam);
        ASSERT_EQ(true, op->HasParameterSets());
    );
}

TEST_F(OperationTest, test_GetGlobalFmCount)
{
    ITERATE_OVER_DEF_DISTS(
        opParam.inActCount = 1;
        opParam.outActCount = 1;
        opParam.fmCount = 10;
        Operation* op = CreateDefaultOperation(opParam);
        ASSERT_EQ(opParam.fmCount, op->GetInput(0)->GetGlobalFmCount());
        ASSERT_EQ(opParam.fmCount, op->GetOutput(0)->GetGlobalFmCount());
    );
}

TEST_F(OperationTest, test_GetInputOutputParamCorrect)
{
    ITERATE_OVER_NETS_OPS_TRS(
        if (inAct) EXPECT_EQ(inAct, mlslOp->GetInput(trIdx));
        if (outAct) EXPECT_EQ(outAct, mlslOp->GetOutput(trIdx));
        EXPECT_EQ(param, mlslOp->GetParameterSet(trIdx));
    );

    ITERATE_OVER_DEF_DISTS(
        opParam.inActCount = 10;
        opParam.outActCount = 20;
        opParam.paramCount = opParam.inActCount * opParam.outActCount;
        Operation* op = CreateDefaultOperation(opParam);
        for (size_t idx = 0; idx < opParam.inActCount; idx++)
            EXPECT_NE((void*)NULL, (void*)(op->GetInput(idx)));
        for (size_t idx = 0; idx < opParam.outActCount; idx++)
            EXPECT_NE((void*)NULL, (void*)(op->GetOutput(idx)));
        for (size_t idx = 0; idx < opParam.paramCount; idx++)
            EXPECT_NE((void*)NULL, (void*)(op->GetParameterSet(idx)));
    );
}

TEST_F(OperationTest, test_GetInputOutputParamOutOfRange)
{
    ITERATE_OVER_NETS_OPS(
        EXPECT_THROW(mlslOp->GetInput(TRIPLET_COUNT), std::out_of_range);
    );

    ITERATE_OVER_NETS_OPS(
        EXPECT_THROW(mlslOp->GetOutput(TRIPLET_COUNT), std::out_of_range);
    );

    ITERATE_OVER_NETS_OPS(
        EXPECT_THROW(mlslOp->GetParameterSet(TRIPLET_COUNT), std::out_of_range);
    );

    ITERATE_OVER_DEF_DISTS(
        Operation* op = CreateDefaultOperation(opParam);
        EXPECT_THROW(op->GetInput(DEFAULT_IN_ACT_COUNT), std::out_of_range);
    );

    ITERATE_OVER_DEF_DISTS(
        Operation* op = CreateDefaultOperation(opParam);
        EXPECT_THROW(op->GetOutput(DEFAULT_OUT_ACT_COUNT), std::out_of_range);
    );

    ITERATE_OVER_DEF_DISTS(
        Operation* op = CreateDefaultOperation(opParam);
        EXPECT_THROW(op->GetParameterSet(DEFAULT_PARAM_COUNT), std::out_of_range);
    );
}

TEST_F(OperationTest, test_RemoveOperations)
{
    ITERATE_OVER_DEF_DISTS(
        Operation* op1 = CreateDefaultOperation(opParam);
        Operation* op2 = CreateDefaultOperation(opParam);
        Operation* op3 = CreateDefaultOperation(opParam);
        EXPECT_EQ(3, defSession->GetOperationCount());
        EXPECT_EQ(op1, defSession->GetOperation(0));
        EXPECT_EQ(op2, defSession->GetOperation(1));
        EXPECT_EQ(op3, defSession->GetOperation(2));
        defSession->RemoveOperations();
        EXPECT_EQ(0, defSession->GetOperationCount());
    );
}

MAIN_FUNCTION();
