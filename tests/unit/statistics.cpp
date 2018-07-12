#include <stdlib.h>
#include "base.hpp"

#define STATS_OUTPUT_FILE "mlsl_stats.log"

class StatisticsTest : public BaseTest
{};

TEST_F(StatisticsTest, test_GetStats)
{
    EXPECT_NE((void*)NULL, (void*)defSession->GetStats());
    ITERATE_OVER_NETS_OPS(
        EXPECT_NE((void*)NULL, (void*)net.mlslSession->GetStats());
    );
}

TEST_F(StatisticsTest, test_StartAndStop)
{
    ITERATE_OVER_DEF_DISTS(
        if (stats->IsEnabled())
        {
            ASSERT_NO_THROW(
                stats->Start();
                EXPECT_EQ(true, stats->IsStarted());
                stats->Stop();
                EXPECT_EQ(false, stats->IsStarted());
            );
        }
    );
}

TEST_F(StatisticsTest, test_Reset)
{
    ITERATE_OVER_DEF_DISTS(
        if (stats->IsEnabled())
        {
            ASSERT_NO_THROW(stats->Reset());
            EXPECT_EQ(0, stats->GetTotalIsolationCommCycles());
            EXPECT_EQ(0, stats->GetTotalCommSize());
            EXPECT_EQ(0, stats->GetTotalCommCycles());
            EXPECT_EQ(0, stats->GetTotalComputeCycles());
        }
    );
}

TEST_F(StatisticsTest, test_Print)
{
    ITERATE_OVER_DEF_DISTS(
        if (stats->IsEnabled())
        {
            ASSERT_NO_THROW(
                stats->Print();
                FILE* outFile = fopen(STATS_OUTPUT_FILE, "r");
                EXPECT_NE((void*)NULL, (void*)outFile);
                fclose(outFile);
            );
        }
    );
}

TEST_F(StatisticsTest, test_GetCycles)
{
    ITERATE_OVER_DEF_DISTS(
        if (stats->IsEnabled())
        {
            ASSERT_EXIT(stats->GetIsolationCommCycles(size_t(-1)), ::testing::ExitedWithCode(1), INVALID_OP_IDX);
        }
    );

    ITERATE_OVER_DEF_DISTS(
        if (stats->IsEnabled())
        {
            ASSERT_EXIT(stats->GetCommSize(size_t(-1)), ::testing::ExitedWithCode(1), INVALID_OP_IDX);
        }
    );

    ITERATE_OVER_DEF_DISTS(
        if (stats->IsEnabled())
        {
            ASSERT_EXIT(stats->GetCommCycles(size_t(-1)), ::testing::ExitedWithCode(1), INVALID_OP_IDX);
        }
    );

    ITERATE_OVER_DEF_DISTS(
        if (stats->IsEnabled())
        {
            ASSERT_EXIT(stats->GetComputeCycles(size_t(-1)), ::testing::ExitedWithCode(1), INVALID_OP_IDX);
        }
    );

    ITERATE_OVER_NETS_OPS(
        if (stats->IsEnabled())
        {
            EXPECT_LT(0, stats->GetIsolationCommCycles(opIdx));
            EXPECT_LT(0, stats->GetCommSize(opIdx));
            EXPECT_EQ(0, stats->GetCommCycles(opIdx));
            EXPECT_EQ(0, stats->GetComputeCycles(opIdx));
        }
    );
}

TEST_F(StatisticsTest, test_UserFlow)
{
    ITERATE_OVER_DEF_DISTS(
        if (stats->IsEnabled())
        {
            ASSERT_NO_THROW(
                if (stats->IsStarted()) stats->Stop();
                EXPECT_EQ(false, stats->IsStarted());

                stats->Reset();
                EXPECT_EQ(0, stats->GetTotalIsolationCommCycles());
                EXPECT_EQ(0, stats->GetTotalCommSize());
                EXPECT_EQ(0, stats->GetTotalCommCycles());
                EXPECT_EQ(0, stats->GetTotalComputeCycles());

                stats->Start();
                EXPECT_EQ(true, stats->IsStarted());
            );
        }
    );
}

MAIN_FUNCTION();
