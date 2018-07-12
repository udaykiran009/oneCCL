#include "base.hpp"

class ActivationTest : public BaseTest
{};

TEST_F(ActivationTest, test_GetGlobalFMCount)
{
    ITERATE_OVER_NETS_OPS_TRS(
        if (inAct) EXPECT_EQ(tr.inActFmCount, inAct->GetGlobalFmCount());
        if (outAct) EXPECT_EQ(tr.outActFmCount, outAct->GetGlobalFmCount());
    );
}

TEST_F(ActivationTest, test_GetGlobalFmOffset)
{
    ITERATE_OVER_NETS_OPS_TRS(
        if (inAct) EXPECT_EQ((tr.inActFmCount / dist.modelProcessCount) * dist.modelProcessIdx, inAct->GetGlobalFmOffset());
        if (outAct)
        {
            if (mlslOp->GetOpType() == OpType::OT_CC)
                EXPECT_EQ(0 /* no offset for outAct of conv */, outAct->GetGlobalFmOffset());
            else
                EXPECT_EQ((tr.outActFmCount / dist.modelProcessCount) * dist.modelProcessIdx, outAct->GetGlobalFmOffset());
        }
    );
}

TEST_F(ActivationTest, test_GetLocalFmCount)
{
    ITERATE_OVER_NETS_OPS_TRS(
        if (inAct) EXPECT_EQ((tr.inActFmCount / dist.modelProcessCount), inAct->GetLocalFmCount());
        if (outAct)
        {
            if (mlslOp->GetOpType() == OpType::OT_CC)
                EXPECT_EQ(tr.outActFmCount /* equal to global count for outAct of conv */, outAct->GetLocalFmCount());
            else
                EXPECT_EQ(tr.outActFmCount / dist.modelProcessCount, outAct->GetLocalFmCount());
        }
    );
}

TEST_F(ActivationTest, test_GetPackBlockCount)
{
    ITERATE_OVER_NETS_OPS_TRS(
        int manyModelParts = ((dist.modelProcessCount > 1) ? 1 : 0);
        if (inAct)
        {
            if (netType == NT_SAME_DISTS) EXPECT_EQ(isMultiProcess * 1 * manyModelParts/* pack block for AllGather on bprop */,
                                                    inAct->GetPackBlockCount());
            if (netType == NT_DIFF_DISTS) EXPECT_EQ(0 /* TODO */, inAct->GetPackBlockCount());
        }
        if (outAct)
        {
            if (netType == NT_SAME_DISTS) EXPECT_EQ(isMultiProcess * dist.modelProcessCount * manyModelParts /* pack blocks for ReduceScatter_block on fprop */,
                                                    outAct->GetPackBlockCount());
            if (netType == NT_DIFF_DISTS) EXPECT_EQ(0 /* TODO */, outAct->GetPackBlockCount());
        }
    );
}

TEST_F(ActivationTest, test_GetUnpackBlockCount)
{
    ITERATE_OVER_NETS_OPS_TRS(
        int manyModelParts = ((dist.modelProcessCount > 1) ? 1 : 0);
        if (inAct)
        {
            if (netType == NT_SAME_DISTS) EXPECT_EQ(isMultiProcess * 1 * manyModelParts /* unpack block for ReduceScatter_block on fprop */,
                                                    inAct->GetUnpackBlockCount());
            if (netType == NT_DIFF_DISTS) EXPECT_EQ(0 /* TODO */, inAct->GetPackBlockCount());
        }
        if (outAct)
        {
            if (netType == NT_SAME_DISTS) EXPECT_EQ(isMultiProcess * dist.modelProcessCount * manyModelParts /* upack blocks for AllGather on bprop */,
                                                    outAct->GetUnpackBlockCount());
            if (netType == NT_DIFF_DISTS) EXPECT_EQ(0 /* TODO */, outAct->GetPackBlockCount());
        }
    );
}

TEST_F(ActivationTest, test_GetDataType)
{
    ITERATE_OVER_NETS_OPS_TRS(
        if (inAct) EXPECT_EQ(MLSL_DTYPE, inAct->GetDataType());
        if (outAct) EXPECT_EQ(MLSL_DTYPE, outAct->GetDataType());
    );
}

TEST_F(ActivationTest, test_GetFmSize)
{
    ITERATE_OVER_NETS_OPS_TRS(
        if (inAct) EXPECT_EQ(tr.inActFmSize, inAct->GetFmSize());
        if (outAct) EXPECT_EQ(tr.outActFmSize, outAct->GetFmSize());
    );
}

TEST_F(ActivationTest, test_SetNextPrevNull)
{
    ITERATE_OVER_DEF_DISTS(
        Operation* op = CreateDefaultOperation(opParam);
        ASSERT_NO_THROW(op->SetNext(NULL, 0, 0));
    );

    ITERATE_OVER_DEF_DISTS(
        Operation* op = CreateDefaultOperation(opParam);
        ASSERT_NO_THROW(op->SetPrev(NULL, 0, 0));
    );
}

TEST_F(ActivationTest, test_SetNext)
{
    /* use the same dists */
    ITERATE_OVER_DEF_DISTS(
        Operation* op1 = CreateDefaultOperation(opParam);
        Operation* op2 = CreateDefaultOperation(opParam);
        ASSERT_NO_THROW(op1->SetNext(op2, 0, 0));
    );

    /* use the different dists */
    ITERATE_OVER_DEF_DISTS(
        Operation* op1 = CreateDefaultOperation(opParam);
        opParam.distType = otherDistType;
        Operation* op2 = CreateDefaultOperation(opParam);
        ASSERT_NO_THROW(op1->SetNext(op2, 0, 0));
    );
}

TEST_F(ActivationTest, test_SetPrev)
{
    /* use the same dists */
    ITERATE_OVER_DEF_DISTS(
        Operation* op1 = CreateDefaultOperation(opParam);
        Operation* op2 = CreateDefaultOperation(opParam);
        ASSERT_NO_THROW(op1->SetPrev(op2, 0, 0));
    );

    /* use the different dists */
    ITERATE_OVER_DEF_DISTS(
        Operation* op1 = CreateDefaultOperation(opParam);
        opParam.distType = otherDistType;
        Operation* op2 = CreateDefaultOperation(opParam);
        ASSERT_NO_THROW(op1->SetPrev(op2, 0, 0));
    );
}

TEST_F(ActivationTest, test_StartWaitComm)
{
    /* fprop */
    ITERATE_OVER_NETS_OPS_TRS(
        ASSERT_NO_THROW(if (inAct) inAct->WaitComm(); if (outAct) outAct->StartComm(outAct->GetCommBuf()););
    );

    /* bprop */
    ITERATE_OVER_NETS_OPS_TRS(
        ASSERT_NO_THROW(if (outAct) outAct->WaitComm(); if (inAct) inAct->StartComm(inAct->GetCommBuf()););
    );
}

TEST_F(ActivationTest, test_CommBlockInfo)
{
    ITERATE_OVER_NETS_OPS_TRS(
        if (inAct)
        {
            size_t localFmCount = (tr.inActFmCount / dist.modelProcessCount);
            PRINT("inAct: localFmCount %zu", localFmCount);

            if (inAct->GetPackBlockCount())
            {
                for (size_t blockIdx = 0; blockIdx < inAct->GetPackBlockCount(); blockIdx++)
                {
                    CommBlockInfo* packBlock = inAct->GetPackBlock(blockIdx);
                    EXPECT_EQ(0, packBlock->GetMbOffset());
                    EXPECT_EQ(mlslOp->GetLocalMinibatchSize(), packBlock->GetMbCount());
                    EXPECT_EQ(0, packBlock->GetFmOffset());
                    EXPECT_EQ(localFmCount, packBlock->GetFmCount());
                    EXPECT_EQ(tr.inActFmSize, packBlock->GetFmSize());
                    EXPECT_EQ(MLSL_DTYPE, packBlock->GetDataType());
                    EXPECT_EQ(dist.modelProcessIdx * mlslOp->GetLocalMinibatchSize() * localFmCount * tr.inActFmSize, packBlock->GetBufOffset());
                }
            }

            if (inAct->GetUnpackBlockCount())
            {
                for (size_t blockIdx = 0; blockIdx < inAct->GetUnpackBlockCount(); blockIdx++)
                {
                    CommBlockInfo* unpackBlock = inAct->GetUnpackBlock(blockIdx);
                    EXPECT_EQ(0, unpackBlock->GetMbOffset());
                    EXPECT_EQ(mlslOp->GetLocalMinibatchSize(), unpackBlock->GetMbCount());
                    EXPECT_EQ(0, unpackBlock->GetFmOffset());
                    EXPECT_EQ(localFmCount, unpackBlock->GetFmCount());
                    EXPECT_EQ(tr.inActFmSize, unpackBlock->GetFmSize());
                    EXPECT_EQ(MLSL_DTYPE, unpackBlock->GetDataType());
                    EXPECT_EQ(0, unpackBlock->GetBufOffset());
                }
            }
        }

        if (outAct)
        {
            size_t localFmCount = (tr.outActFmCount / dist.modelProcessCount);
            PRINT("outAct: localFmCount %zu", localFmCount);

            if (outAct->GetPackBlockCount())
            {
                for (size_t blockIdx = 0; blockIdx < outAct->GetPackBlockCount(); blockIdx++)
                {
                    CommBlockInfo* packBlock = outAct->GetPackBlock(blockIdx);
                    EXPECT_EQ(0, packBlock->GetMbOffset());
                    EXPECT_EQ(mlslOp->GetLocalMinibatchSize(), packBlock->GetMbCount());
                    EXPECT_EQ(blockIdx * localFmCount, packBlock->GetFmOffset());
                    EXPECT_EQ(localFmCount, packBlock->GetFmCount());
                    EXPECT_EQ(tr.outActFmSize, packBlock->GetFmSize());
                    EXPECT_EQ(MLSL_DTYPE, packBlock->GetDataType());
                    EXPECT_EQ(blockIdx * mlslOp->GetLocalMinibatchSize() * localFmCount * tr.outActFmSize, packBlock->GetBufOffset());
                }
            }

            if (outAct->GetUnpackBlockCount())
            {
                for (size_t blockIdx = 0; blockIdx < outAct->GetUnpackBlockCount(); blockIdx++)
                {
                    CommBlockInfo* unpackBlock = outAct->GetUnpackBlock(blockIdx);
                    EXPECT_EQ(0, unpackBlock->GetMbOffset());
                    EXPECT_EQ(mlslOp->GetLocalMinibatchSize(), unpackBlock->GetMbCount());
                    EXPECT_EQ(blockIdx * localFmCount, unpackBlock->GetFmOffset());
                    EXPECT_EQ(localFmCount, unpackBlock->GetFmCount());
                    EXPECT_EQ(tr.outActFmSize, unpackBlock->GetFmSize());
                    EXPECT_EQ(MLSL_DTYPE, unpackBlock->GetDataType());
                    EXPECT_EQ(blockIdx * mlslOp->GetLocalMinibatchSize() * localFmCount * tr.outActFmSize, unpackBlock->GetBufOffset());
                }
            }
        }
    );
}

MAIN_FUNCTION();
