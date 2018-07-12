#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>
#include <sys/syscall.h>

#include "gtest/gtest.h"

#include "mlsl.hpp"

using namespace std;
using namespace MLSL;

#define gettid() syscall(SYS_gettid)

#if ENABLE_DEBUG

#define PRINT(fmt, ...)                               \
  do {                                                \
      fflush(stdout);                                 \
      printf("\n(%ld): %s: " fmt "\n",                \
              gettid(), __FUNCTION__, ##__VA_ARGS__); \
      fflush(stdout);                                 \
  } while (0)

#define PRINT_BUFFER(buf, bufSize, prefix)           \
  do {                                               \
      std::string strToPrint;                        \
      for (size_t idx = 0; idx < bufSize; idx++)     \
      {                                              \
          strToPrint += to_string(buf[idx]);         \
          if (idx != bufSize - 1)                    \
              strToPrint += ", ";                    \
      }                                              \
      strToPrint = std::string(prefix) + strToPrint; \
      PRINT("%s", strToPrint.c_str());               \
  } while (0)

#else /* ENABLE_DEBUG */

#define PRINT(fmt, ...) {}
#define PRINT_BUFFER(buf, bufSize, prefix) {}

#endif /* ENABLE_DEBUG */

#define ASSERT(cond, fmt, ...)                                                       \
  do                                                                                 \
  {                                                                                  \
      if (!(cond))                                                                   \
      {                                                                              \
          fprintf(stderr, "(%ld): %s:%s:%d: ASSERT '%s' FAILED: " fmt "\n",          \
                  gettid(), __FILE__, __FUNCTION__, __LINE__, #cond, ##__VA_ARGS__); \
          fflush(stderr);                                                            \
      }                                                                              \
  } while(0)

#define DTYPE                 float
#define DTYPE_SIZE            sizeof(DTYPE)
#define GLOBAL_MINIBATCH_SIZE 16
#define CACHELINE_SIZE        64

template<class TYPE>
string to_string(TYPE value)
{
    stringstream stream;
    stream << value;
    return stream.str();
}

DataType get_type(size_t size)
{
    switch(size)
    {
        case 8 : return DT_DOUBLE;
        case 4 : return DT_FLOAT;
        case 1 : return DT_BYTE;
        default: {
                PRINT("There is no information about which data type corresponds to this size");
                return (DataType)-1;
        }
    }
}

typedef DTYPE* DTypePtr;

class DataParallelismTest : public ::testing::Test
{
public:
    DTypePtr paramBuf, paramGradBuf, paramCommBuf, resultParamBuf, expectedParamBuf;
    Session* session;
    Distribution* distribution;
    Operation* operation;
    OperationRegInfo* opRegInfo;

    void SetUp()
    {
        session = Environment::GetEnv().CreateSession();
        session->SetGlobalMinibatchSize(GLOBAL_MINIBATCH_SIZE);
        distribution = Environment::GetEnv().CreateDistribution(Environment::GetEnv().GetProcessCount(), 1); /* data parallelism only */
        opRegInfo = session->CreateOperationRegInfo(OT_CC);
        opRegInfo->SetName("MyLayerName");
    }

    void TearDown()
    {
        Environment::GetEnv().Free(paramBuf);
        Environment::GetEnv().Free(paramGradBuf);
        Environment::GetEnv().Free(paramCommBuf);
        Environment::GetEnv().Free(resultParamBuf);
        Environment::GetEnv().Free(expectedParamBuf);

        session->DeleteOperationRegInfo(opRegInfo);
        Environment::GetEnv().DeleteSession(session);
        Environment::GetEnv().DeleteDistribution(distribution);
    }

    void CreateOperation(bool distUpdate)
    {
        opRegInfo->AddParameterSet(Environment::GetEnv().GetProcessCount() * 2, /* kernel count */
                                   2, /* kernel size */
                                   get_type(DTYPE_SIZE),
                                   distUpdate);
        size_t opIdx = session->AddOperation(opRegInfo, distribution);
        session->Commit();
        operation = session->GetOperation(opIdx);
    }

    void CreateBuffers(size_t paramSize)
    {
        size_t bytesToAlloc = paramSize * DTYPE_SIZE;

        paramBuf = (DTypePtr)Environment::GetEnv().Alloc(bytesToAlloc, CACHELINE_SIZE);
        paramGradBuf = (DTypePtr)Environment::GetEnv().Alloc(bytesToAlloc, CACHELINE_SIZE);
        paramCommBuf = (DTypePtr)Environment::GetEnv().Alloc(bytesToAlloc, CACHELINE_SIZE);
        resultParamBuf = (DTypePtr)Environment::GetEnv().Alloc(bytesToAlloc, CACHELINE_SIZE);
        expectedParamBuf = (DTypePtr)Environment::GetEnv().Alloc(bytesToAlloc, CACHELINE_SIZE);

        for (size_t idx = 0; idx < paramSize; idx++)
        {
            paramBuf[idx] = (idx + 2) * 3;
            paramGradBuf[idx] = idx + 1;
            paramCommBuf[idx] = paramGradBuf[idx];
        }

        PRINT_BUFFER(paramBuf, paramSize, "paramBuf: ");
        PRINT_BUFFER(paramGradBuf, paramSize, "paramGradBuf: ");
    }

    int UpdateParameterSet()
    {
        CreateOperation(false);
        size_t paramSize = operation->GetParameterSet(0)->GetLocalKernelCount() * operation->GetParameterSet(0)->GetKernelSize();

        CreateBuffers(paramSize);

        /* exchange parameter gradient */
        operation->GetParameterSet(0)->StartGradientComm(paramCommBuf);
        DTypePtr retBuf = (DTypePtr)operation->GetParameterSet(0)->WaitGradientComm();
        ASSERT(!retBuf || paramCommBuf == retBuf, "buffer from WaitGradientComm should be the same as in StartGradientComm");

        /* calculate the result parameter set */
        for (size_t idx = 0; idx < paramSize; idx++)
            resultParamBuf[idx] = paramBuf[idx] + paramCommBuf[idx];

        /* calculate the expected parameter set */
        for (size_t idx = 0; idx < paramSize; idx++)
            expectedParamBuf[idx] = paramBuf[idx] + paramGradBuf[idx] * Environment::GetEnv().GetProcessCount();

        PRINT_BUFFER(paramBuf, paramSize, "do check: paramBuf: ");
        PRINT_BUFFER(paramGradBuf, paramSize, "do check: paramGradBuf: ");
        PRINT_BUFFER(paramCommBuf, paramSize, "do check: paramCommBuf after Start/WaitGradientComm: ");
        PRINT_BUFFER(resultParamBuf, paramSize, "do check: resultParamBuf: ");
        PRINT_BUFFER(expectedParamBuf, paramSize, "do check: expectedParamBuf: ");

        /* compare the result and expected parameter sets  */
        int compareStatus = memcmp(resultParamBuf, expectedParamBuf, paramSize * DTYPE_SIZE);
        return compareStatus;
    }

    int UpdateParameterSetDist()
    {
        CreateOperation(true);
        size_t paramSize = operation->GetParameterSet(0)->GetLocalKernelCount() * operation->GetParameterSet(0)->GetKernelSize();
        size_t ownedParamSize = operation->GetParameterSet(0)->GetOwnedKernelCount() * operation->GetParameterSet(0)->GetKernelSize();

        CreateBuffers(paramSize);

        /* exchange parameter gradient */
        operation->GetParameterSet(0)->StartGradientComm(paramCommBuf);
        DTypePtr retBuf = (DTypePtr)operation->GetParameterSet(0)->WaitGradientComm();

        PRINT_BUFFER(paramCommBuf, paramSize, "paramCommBuf after Start/WaitGradientComm: ");
        if (retBuf) PRINT_BUFFER(retBuf, paramSize, "retBuf after Start/WaitGradientComm: ");

        /* calculate the expected gradient */
        if ((retBuf != NULL && paramCommBuf != retBuf) ||
            (operation->GetDistribution()->GetProcessIdx(GT_DATA) > 0 &&
            operation->GetParameterSet(0)->GetOwnedKernelCount() != operation->GetParameterSet(0)->GetLocalKernelCount()))
        {
            PRINT("do data shiftment after WaitGradientComm");
            memcpy(paramCommBuf + operation->GetParameterSet(0)->GetOwnedKernelOffset() * operation->GetParameterSet(0)->GetKernelSize(),
                   retBuf,
                   ownedParamSize * DTYPE_SIZE);
        }

        PRINT_BUFFER(paramCommBuf, paramSize, "paramCommBuf after data shiftment: ");

        /* exchange parameter increment */
        operation->GetParameterSet(0)->StartIncrementComm(paramCommBuf);
        retBuf = (DTypePtr)operation->GetParameterSet(0)->WaitIncrementComm();
        ASSERT(!retBuf || paramCommBuf == retBuf, "buffer from WaitIncrementComm should be the same as in StartIncrementComm");

        PRINT_BUFFER(paramCommBuf, paramSize, "do check: paramCommBuf after Start/WaitIncrementComm: ");

        /* calculate the result parameter set */
        for (size_t idx = 0; idx < paramSize; idx++)
            resultParamBuf[idx] = paramBuf[idx] + paramCommBuf[idx];

        /* calculate the expected parameter set */
        for (size_t idx = 0; idx < paramSize; idx++)
            expectedParamBuf[idx] = paramBuf[idx] + paramGradBuf[idx] * Environment::GetEnv().GetProcessCount();

        PRINT_BUFFER(paramBuf, paramSize, "do check: paramBuf: ");
        PRINT_BUFFER(paramGradBuf, paramSize, "do check: paramGradBuf: ");
        PRINT_BUFFER(resultParamBuf, paramSize, "do check: resultParamBuf: ");
        PRINT_BUFFER(expectedParamBuf, paramSize, "do check: expectedParamBuf: ");

        /* compare the result and expected parameter sets  */
        int compareStatus = memcmp(resultParamBuf, expectedParamBuf, paramSize * DTYPE_SIZE);

        return compareStatus;
    }
};

TEST_F(DataParallelismTest, test_UpdateParameterSet)
{
    EXPECT_EQ(0, UpdateParameterSet());
}

TEST_F(DataParallelismTest, test_UpdateParameterSetDist)
{
    EXPECT_EQ(0, UpdateParameterSetDist());
}

int main(int argc, char **argv)
{
    Environment::GetEnv().Init(&argc, &argv);

    if (Environment::GetEnv().GetProcessCount() > 1)
    {
        /* This code need for XML logging. Different process write to different XML files. */
        for (int idx = 1; idx < argc; idx++)
        {
            if (strstr(argv[idx], "--gtest_output"))
            {
                std::string output;
                std::string s = std::string(argv[idx]);
                size_t pos = s.find(".xml");
                /* len (--gtest_output=xml:) = 19 */
                output = s.substr(15, pos - 1) + "_" + std::to_string((long long)Environment::GetEnv().GetProcessIdx()) + ".xml";
                argv[idx] = '\0';
                ::testing::GTEST_FLAG(output) = output.c_str();
            }

        }
    }

    int result = 0;
    ::testing::InitGoogleTest(&argc, argv);
    result = RUN_ALL_TESTS();

    Environment::GetEnv().Finalize();

    return result;
}
