#include <stdlib.h>

#include "base.hpp"

class DistributionTest : public BaseTest
{};

TEST_F(DistributionTest, test_CreateDistributionCorrect)
{
    Distribution* dist1 = Environment::GetEnv().CreateDistribution(2, 1);
    Distribution* dist2 = Environment::GetEnv().CreateDistribution(2, 4);
    Distribution* dist3 = Environment::GetEnv().CreateDistribution(1, 10);

    EXPECT_NE(NULL, dist1 && dist2 && dist3);

    Environment::GetEnv().DeleteDistribution(dist1);
    Environment::GetEnv().DeleteDistribution(dist2);
    Environment::GetEnv().DeleteDistribution(dist3);
}

TEST_F(DistributionTest, test_CreateDistributionAssert)
{
    ASSERT_EXIT(Environment::GetEnv().CreateDistribution(size_t(-2), 1), ::testing::ExitedWithCode(1), PARTS_SHOULD_BE_POSITIVE);
    ASSERT_EXIT(Environment::GetEnv().CreateDistribution(2, size_t(-1)), ::testing::ExitedWithCode(1), PARTS_SHOULD_BE_POSITIVE);
    ASSERT_EXIT(Environment::GetEnv().CreateDistribution(size_t(-2), size_t(-1)), ::testing::ExitedWithCode(1), PARTS_SHOULD_BE_POSITIVE);
    ASSERT_EXIT(Environment::GetEnv().CreateDistribution(0, 1), ::testing::ExitedWithCode(1), PARTS_SHOULD_BE_POSITIVE);
    ASSERT_EXIT(Environment::GetEnv().CreateDistribution(2, 0), ::testing::ExitedWithCode(1), PARTS_SHOULD_BE_POSITIVE);
}

TEST_F(DistributionTest, test_SetGetMinibatchSize)
{
    size_t validMbSize = 4;
    Session* session = Environment::GetEnv().CreateSession();
    ASSERT_EXIT(session->SetGlobalMinibatchSize(0), ::testing::ExitedWithCode(1), MB_SIZE_SHOULD_BE_POSITIVE);
    ASSERT_EXIT(session->SetGlobalMinibatchSize(size_t(-1)), ::testing::ExitedWithCode(1), MB_SIZE_SHOULD_BE_POSITIVE);
    ASSERT_NO_THROW(session->SetGlobalMinibatchSize(validMbSize));
    ASSERT_EXIT(session->SetGlobalMinibatchSize(validMbSize), ::testing::ExitedWithCode(1), MB_CAN_BE_SET_ONLY_ONCE);
    EXPECT_EQ(validMbSize, session->GetGlobalMinibatchSize());
    Environment::GetEnv().DeleteSession(session);
}

TEST_F(DistributionTest, test_CommitSessionWithoutMBSize)
{
    Session* session = Environment::GetEnv().CreateSession();
    ASSERT_EXIT(session->Commit(), ::testing::ExitedWithCode(1), SET_GLOBAL_MB_SIZE);
    Environment::GetEnv().DeleteSession(session);
}

TEST_F(DistributionTest, test_CommitSession)
{
    Session* session = Environment::GetEnv().CreateSession();
    session->SetGlobalMinibatchSize(1);
    ASSERT_NO_THROW(session->Commit());
    ASSERT_EXIT(session->Commit(), ::testing::ExitedWithCode(1), COMMIT_SHOULD_BE_CALLED_ONCE);
    ASSERT_EXIT(session->Commit(), ::testing::ExitedWithCode(1), COMMIT_SHOULD_BE_CALLED_ONCE);
    Environment::GetEnv().DeleteSession(session);
}

TEST_F(DistributionTest, test_GetProcessIdx)
{
    EXPECT_EQ(defDistributions[DT_DATA]->GetProcessIdx(GroupType::GT_GLOBAL),
              defDistributions[DT_DATA]->GetProcessIdx(GroupType::GT_DATA));

    EXPECT_EQ(0,
              defDistributions[DT_DATA]->GetProcessIdx(GroupType::GT_MODEL));

    EXPECT_EQ(defDistributions[DT_MODEL]->GetProcessIdx(GroupType::GT_GLOBAL),
              defDistributions[DT_MODEL]->GetProcessIdx(GroupType::GT_MODEL));

    EXPECT_EQ(0,
              defDistributions[DT_MODEL]->GetProcessIdx(GroupType::GT_DATA));
}

TEST_F(DistributionTest, test_GetProcessCount)
{
    EXPECT_EQ(defDistributions[DT_DATA]->GetProcessCount(GroupType::GT_GLOBAL),
              defDistributions[DT_DATA]->GetProcessCount(GroupType::GT_DATA));

    EXPECT_EQ(1,
              defDistributions[DT_DATA]->GetProcessCount(GroupType::GT_MODEL));

    EXPECT_EQ(defDistributions[DT_MODEL]->GetProcessCount(GroupType::GT_GLOBAL),
              defDistributions[DT_MODEL]->GetProcessCount(GroupType::GT_MODEL));

    EXPECT_EQ(1,
              defDistributions[DT_MODEL]->GetProcessCount(GroupType::GT_DATA));

    if (isMultiProcess)
        EXPECT_EQ(0,
                  defDistributions[DT_HYBRID]->GetProcessCount(GroupType::GT_GLOBAL) % 2);
    else
        EXPECT_EQ(1,
                  defDistributions[DT_HYBRID]->GetProcessCount(GroupType::GT_GLOBAL));

    EXPECT_EQ(defDistributions[DT_HYBRID]->GetProcessCount(GroupType::GT_GLOBAL),
              defDistributions[DT_HYBRID]->GetProcessCount(GroupType::GT_DATA) *
              defDistributions[DT_HYBRID]->GetProcessCount(GroupType::GT_MODEL));
}

MAIN_FUNCTION();
