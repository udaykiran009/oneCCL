#ifndef BASE_HPP
#define BASE_HPP

#include <sys/syscall.h> /* syscall */

#include "gtest/gtest.h"

#include "mlsl.hpp"
#include <mpi.h>
#include <cstdlib> /* malloc */

#include <ctime>

using namespace MLSL;
using namespace std;

#define TIMEOUT 400

#define GETTID() syscall(SYS_gettid)

#if 0

#define PRINT(fmt, ...)                               \
  do {                                                \
      fflush(stdout);                                 \
      printf("\n(%ld): %s: " fmt "\n",                \
              GETTID(), __FUNCTION__, ##__VA_ARGS__); \
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

#if defined(__INTEL_COMPILER) || defined(__ICC)
#define MY_MALLOC(size) _mm_malloc(size, 64)
#define MY_FREE(ptr)    _mm_free(ptr)
#elif defined(__GNUC__)
#define MY_MALLOC(size) malloc(size)
#define MY_FREE(ptr)    free(ptr)
#else
# error "this compiler is not supported"
#endif
//TODO: Fix me (< 1)
#define OUTPUT_NAME_ARG "--gtest_output="
#define PATCH_OUTPUT_NAME_ARG(argc, argv)                                               \
  do {                                                                                  \
      if (Environment::GetEnv().GetProcessCount() > 1)                                  \
      {                                                                                 \
          for (int idx = 1; idx < argc; idx++)                                          \
          {                                                                             \
              if (strstr(argv[idx], OUTPUT_NAME_ARG))                                   \
              {                                                                         \
                  string patchedArg;                                                    \
                  string originArg = string(argv[idx]);                                 \
                  size_t extPos = originArg.find(".xml");                               \
                  size_t argLen = strlen(OUTPUT_NAME_ARG);                              \
                  patchedArg = originArg.substr(argLen, extPos - argLen) + "_"          \
                      + to_string(Environment::GetEnv().GetProcessIdx())                \
                      + ".xml";                                                         \
                  PRINT("originArg %s, extPos %zu, argLen %zu, patchedArg %s",          \
                         originArg.c_str(), extPos, argLen, patchedArg.c_str());        \
                  argv[idx] = '\0';                                                     \
                  if (Environment::GetEnv().GetProcessIdx())                            \
                      ::testing::GTEST_FLAG(output) = "";                               \
                  else                                                                  \
                      ::testing::GTEST_FLAG(output) = patchedArg.c_str();               \
              }                                                                         \
          }                                                                             \
      }                                                                                 \
  } while (0)

void PrintErrMessage(char* errMessage, std::ostream &output)
{
    int messageLen = strlen(errMessage);
    int numProc, myRank;
    MPI_Comm_size(MPI_COMM_WORLD, &numProc);
    MPI_Comm_rank(MPI_COMM_WORLD, &myRank);
    int* arrMessageLen = new int[numProc];
    MPI_Allgather(&messageLen, 1, MPI_INT, arrMessageLen, 1, MPI_INT, MPI_COMM_WORLD);
    int fullMessageLen = 0;
    for (int i = 0; i < numProc; i++)
        fullMessageLen += arrMessageLen[i];
    if (fullMessageLen == 0)
    {
        delete[] arrMessageLen;
        return;
    }
    char* arrErrMessage = new char[fullMessageLen];
    int* displs = new int[numProc];
    displs[0] = 0;
    for (int i = 1; i < numProc; i++)
        displs[i] = displs[i - 1] + arrMessageLen[i - 1];
    MPI_Gatherv(errMessage, messageLen, MPI_CHAR, arrErrMessage, arrMessageLen, displs, MPI_CHAR, 0, MPI_COMM_WORLD);
    if (myRank == 0)
    {
        output << arrErrMessage;
    }
    delete[] arrMessageLen;
    delete[] arrErrMessage;
    delete[] displs;
}

#define RUN_METHOD_DEFINITION(ClassName)                      \
  template <typename T>                                       \
  int MainTest::Run(TestParam tParam)                         \
  {                                                           \
      ClassName<T> className;                                 \
      int timeout = TIMEOUT;                                  \
      char* timeoutEnv = getenv("MLSL_FUNC_TEST_TIMEOUT");    \
      if (timeoutEnv)                                         \
          timeout = atoi(timeoutEnv);                         \
      TypedTestParam<T> typedParam(tParam);                   \
      clock_t start = clock();                                \
      int result = className.Run(typedParam);                 \
      clock_t end = clock();                                  \
      double time = (end - start) / CLOCKS_PER_SEC;           \
      MPI_Allreduce(MPI_IN_PLACE, &result, 1,                 \
                    MPI_INT, MPI_LOR, MPI_COMM_WORLD);        \
      if ((result != TEST_SUCCESS) || (time >= timeout))      \
      {                                                       \
          std::ostringstream output;                          \
          PrintErrMessage(className.GetErrMessage(), output); \
          if (typedParam.processIdx == 0)                     \
          {                                                   \
              output << "over timeout=" << time << endl;      \
              typedParam.Print(output);                       \
              printf("%s", output.str().c_str());             \
              EXPECT_TRUE(false) << output.str();             \
          }                                                   \
          output.str("");                                     \
          output.clear();                                     \
          return TEST_FAILURE;                                \
      }                                                       \
      return TEST_SUCCESS;                                    \
  }

#define ASSERT(cond, fmt, ...)                                                       \
  do                                                                                 \
  {                                                                                  \
      if (!(cond))                                                                   \
      {                                                                              \
          fprintf(stderr, "(%ld): %s:%s:%d: ASSERT '%s' FAILED: " fmt "\n",          \
                  GETTID(), __FILE__, __FUNCTION__, __LINE__, #cond, ##__VA_ARGS__); \
          fflush(stderr);                                                            \
          Environment::GetEnv().Finalize();                                          \
          exit(0);                                                                   \
      }                                                                              \
  } while(0)

#define MAIN_FUNCTION()                         \
  int main(int argc, char **argv)               \
  {                                             \
      InitTestParams();                         \
      Environment::GetEnv().Init(&argc, &argv); \
      PATCH_OUTPUT_NAME_ARG(argc, argv);        \
      ::testing::InitGoogleTest(&argc, argv);   \
      int res = RUN_ALL_TESTS();                \
      Environment::GetEnv().Finalize();         \
      return res;                               \
  }

#define UNUSED_ATTR __attribute__((unused))

#define TEST_SUCCESS 0
#define TEST_FAILURE 1

#define TEST_CASES_DEFINITION(FuncName)                             \
  TEST_P(MainTest, FuncName) {                                      \
      TestParam param = GetParam();                                 \
      EXPECT_EQ(TEST_SUCCESS, this->Test(param));                   \
  }                                                                 \
  INSTANTIATE_TEST_CASE_P(TestWithParameters,                       \
                          MainTest,                                 \
                          ::testing::ValuesIn(testParams));

#define POST_AND_PRE_INCREMENTS(EnumName, LAST_ELEM)                                                                   \
  EnumName& operator++(EnumName& orig) { if (orig != LAST_ELEM) orig = static_cast<EnumName>(orig + 1); return orig; } \
  EnumName operator++(EnumName& orig, int) { EnumName rVal = orig; ++orig; return rVal; }

#define SOME_VALUE (0xdeadbeef)

#define SELF_COMM(param) (((param.GetGroupType() == TGT_DATA && param.GetDistType() == DIST_TYPE_MODEL)                   \
                         || (param.GetGroupType() == TGT_MODEL && param.GetDistType() == DIST_TYPE_DATA)) ? true : false)

#define CHECK_SELF_COMM(param)                                                                                    \
  if (SELF_COMM(param))                                                                                           \
  {                                                                                                               \
      for (size_t i = 0; i < param.elemCount; i++)                                                                \
      {                                                                                                           \
          if (param.recvBuf[i] != param.sendBuf[i])                                                               \
          {                                                                                                       \
              sprintf(this->errMessage, "self_comm: [%zu] got recvBuf[%zu] = %f, but expected = %f\n",            \
                                        param.processIdx, i, (double)param.recvBuf[i], (double)param.sendBuf[i]); \
              return TEST_FAILURE;                                                                                \
          }                                                                                                       \
      }                                                                                                           \
      return TEST_SUCCESS;                                                                                        \
  }

#define ROOT_PROCESS_IDX (0)

template<class TYPE>
string to_string(TYPE value)
{
    stringstream stream;
    stream << value;
    return stream.str();
}

typedef enum
{
    DIST_TYPE_DATA   = 0,
    DIST_TYPE_MODEL  = 1,
    DIST_TYPE_HYBRID = 2,
    DIST_TYPE_LAST
} DistType;
DistType firstDistType = DIST_TYPE_DATA;
map<int, const char*> distTypeStr = { {DIST_TYPE_DATA,   "DATA"},
                                      {DIST_TYPE_MODEL,  "MODEL"},
                                      {DIST_TYPE_HYBRID, "HYBRID"} };

typedef enum
{
    TGT_DATA   = GT_DATA,
    TGT_MODEL  = GT_MODEL,
    TGT_GLOBAL = GT_GLOBAL,
    TGT_LAST
} TestGroupType;
TestGroupType firstGroupType = TGT_DATA;
map<int, const char*> groupTypeStr = { {TGT_DATA,   "DATA"},
                                       {TGT_MODEL,  "MODEL"},
                                       {TGT_GLOBAL, "GLOBAL"} };

typedef enum
{
    PT_OOP = 0,
    PT_IN  = 1,
    PT_LAST
} PlaceType;
PlaceType firstPlaceType = PT_OOP;
map<int, const char*> placeTypeStr = { {PT_OOP, "OUT_OF_PLACE"},
                                       {PT_IN,  "IN_PLACE"} };

typedef enum
{
    BT_MLSL = 0,
    BT_USER = 1,
    BT_LAST
} BufType;
BufType firstBufType = BT_MLSL;
map<int, const char*> bufTypeStr = { {BT_MLSL, "MLSL"},
                                     {BT_USER, "USER"} };

typedef enum
{
    ST_SMALL  = 0,
    ST_MEDIUM = 1,
    ST_LARGE  = 2,
    ST_LAST
} SizeType;
SizeType firstSizeType = ST_SMALL;
map<int, const char*> sizeTypeStr = { {ST_SMALL,  "SMALL"},
                                      {ST_MEDIUM, "MEDIUM"},
                                      {ST_LARGE,  "LARGE"} };

map<int, size_t> sizeTypeValues = { {ST_SMALL,  16},
                                    {ST_MEDIUM, 32768},
                                    {ST_LARGE,  524288} };

typedef enum
{
    CT_WAIT = 0,
    CT_TEST = 1,
    CT_LAST
} CompletionType;
CompletionType firstCompletionType = CT_WAIT;
map<int, const char*> completionTypeStr = { {CT_WAIT, "WAIT"},
                                            {CT_TEST, "TEST"} };

typedef enum
{
    TDT_FLOAT  = DT_FLOAT,
    TDT_DOUBLE = DT_DOUBLE,
//    TDT_BYTE   = DT_BYTE,
    TDT_LAST
} TestDataType;
TestDataType firstDataType = TDT_FLOAT;
map<int, const char*> dataTypeStr = { {TDT_FLOAT,  "FLOAT"},
                                      {TDT_DOUBLE, "DOUBLE"} };//,
//                                      {TDT_BYTE,   "BYTE"} };

POST_AND_PRE_INCREMENTS(DistType, DIST_TYPE_LAST);
POST_AND_PRE_INCREMENTS(TestGroupType, TGT_LAST);
POST_AND_PRE_INCREMENTS(PlaceType, PT_LAST);
POST_AND_PRE_INCREMENTS(BufType, BT_LAST);
POST_AND_PRE_INCREMENTS(SizeType, ST_LAST);
POST_AND_PRE_INCREMENTS(CompletionType, CT_LAST);
POST_AND_PRE_INCREMENTS(TestDataType, TDT_LAST);

//This struct needed for gtest
struct TestParam
{
    TestParam() {}
    TestParam(TestParam* const tParam)
    {
        distType = tParam->distType;
        groupType = tParam->groupType;
        placeType = tParam->placeType;
        bufType = tParam->bufType;
        sizeType = tParam->sizeType;
        completionType = tParam->completionType;
        dataType = tParam->dataType;
    }
    DistType distType;
    TestGroupType groupType;
    PlaceType placeType;
    BufType bufType;
    SizeType sizeType;
    CompletionType completionType;
    TestDataType dataType;
};

#define TEST_COUNT (DIST_TYPE_LAST * TGT_LAST * PT_LAST * BT_LAST * ST_LAST * CT_LAST * TDT_LAST)

TestParam testParams[TEST_COUNT];

void InitTestParams()
{
    size_t idx = 0;
    for (DistType distType = firstDistType; distType < DIST_TYPE_LAST; distType++)
    {
        for (TestGroupType groupType = firstGroupType; groupType < TGT_LAST; groupType++)
        {
            for (PlaceType placeType = firstPlaceType; placeType < PT_LAST; placeType++)
            {
                for (BufType bufType = firstBufType; bufType < BT_LAST; bufType++)
                {
                    for (SizeType sizeType = firstSizeType; sizeType < ST_LAST; sizeType++)
                    {
                        for (CompletionType completionType = firstCompletionType; completionType < CT_LAST; completionType++)
                        {
                            for (TestDataType dataType = firstDataType; dataType < TDT_LAST; dataType++)
                            {
                                testParams[idx].distType = distType;
                                testParams[idx].groupType = groupType;
                                testParams[idx].placeType = placeType;
                                testParams[idx].bufType = bufType;
                                testParams[idx].sizeType = sizeType;
                                testParams[idx].completionType = completionType;
                                testParams[idx].dataType = dataType;
                                idx++;
                            }
                        }
                    }
                }
            }
        }
    }
}

template <typename T>
struct TypedTestParam
{
    TestParam testParam;
    Distribution* dist;
    size_t elemCount;
    size_t processCount;
    size_t processIdx;
    T* sendBuf;
    T* recvBuf;

    TypedTestParam(TestParam tParam) : testParam(tParam)
    {
        dist = NULL;
        CreateDistribution();
        elemCount = GetElemCount();
        processCount = dist->GetProcessCount((GroupType)testParam.groupType);
        processIdx = dist->GetProcessIdx((GroupType)testParam.groupType);
        sendBuf = AllocBuf();
        if (testParam.placeType == PT_OOP) recvBuf = AllocBuf();
        else recvBuf = sendBuf;
    }

    ~TypedTestParam()
    {
        FreeBuf(sendBuf);
        if (testParam.placeType == PT_OOP) FreeBuf(recvBuf);
        Environment::GetEnv().DeleteDistribution(dist);
    }

    T* AllocBuf()
    {
        size_t bytesToAlloc = elemCount * processCount * sizeof(T);
        if (testParam.bufType == BT_MLSL) return (T*)(Environment::GetEnv().Alloc(bytesToAlloc, 64));
        else return (T*)MY_MALLOC(bytesToAlloc);
    }

    void FreeBuf(T* buf)
    {
        if (testParam.bufType == BT_MLSL) Environment::GetEnv().Free(buf);
        else MY_FREE(buf);
    }

    void CreateDistribution()
    {
        size_t modelParts = 2;
        size_t globalProcessCount = Environment::GetEnv().GetProcessCount();
        ASSERT(globalProcessCount % modelParts == 0, "globalProcessCount (%zu) should be divisible by modelParts (%zu)",
               globalProcessCount, modelParts);
        if (testParam.distType == DIST_TYPE_DATA)
            dist = Environment::GetEnv().CreateDistribution(globalProcessCount, 1);
        else if (testParam.distType == DIST_TYPE_MODEL)
            dist = Environment::GetEnv().CreateDistribution(1, globalProcessCount);
        else if (testParam.distType == DIST_TYPE_HYBRID)
            dist = Environment::GetEnv().CreateDistribution(Environment::GetEnv().GetProcessCount() / modelParts, modelParts);
        EXPECT_TRUE(dist) << "Unknown dist type: " << testParam.distType;
    }

    void CompleteRequest(CommReq* req)
    {
        bool isCompleted = false;
        if  (testParam.completionType == CT_TEST)
            do { Environment::GetEnv().Test(req, &isCompleted); } while (!isCompleted);
        else if (testParam.completionType == CT_WAIT)
            Environment::GetEnv().Wait(req);
        else ASSERT(0, "unexpected completion type %d", testParam.completionType);
    }

    void Print(std::ostream &output)
    {
        char strParameters[1000];
        memset(strParameters, '\0', 1000);

        sprintf(strParameters, "test_param:\ndataType %s\ndistType %s\ngroupType %s\nplaceType %s\nbufType %s\nsizeType %s\ncompletionType %s\n"
                               "dist %p\nelemCount %zu\nprocessCount %zu\nprocessIdx %zu\n"
                               "sendBuf %p\nrecvBuf %p\n-------------\n",
                               GetDataTypeStr(), GetDistTypeStr(), GetGroupTypeStr(), GetPlaceTypeStr(),
                               GetBufTypeStr(), GetSizeTypeStr(), GetCompletionTypeStr(),
                               dist, elemCount, processCount, processIdx, sendBuf, recvBuf);
        output << strParameters;

        fflush(stdout);
    }

    const char* GetDistTypeStr() { return distTypeStr[testParam.distType]; }
    const char* GetGroupTypeStr() { return groupTypeStr[testParam.groupType]; }
    const char* GetPlaceTypeStr() { return placeTypeStr[testParam.placeType]; }
    const char* GetBufTypeStr() { return bufTypeStr[testParam.bufType]; }
    const char* GetSizeTypeStr() { return sizeTypeStr[testParam.sizeType]; }
    const char* GetCompletionTypeStr() { return completionTypeStr[testParam.completionType]; }
    const char* GetDataTypeStr() { return dataTypeStr[testParam.dataType]; }
    size_t GetElemCount() { return sizeTypeValues[testParam.sizeType]; }
    PlaceType GetPlaceType() { return testParam.placeType; }
    TestGroupType GetGroupType() { return testParam.groupType; }
    DistType GetDistType() { return testParam.distType; }
    SizeType GetSizeType() { return testParam.sizeType; }
    TestDataType GetDataType() { return testParam.dataType; }
};

template <typename T>
class BaseTest
{

public:
    size_t globalProcessIdx;
    size_t globalProcessCount;

    void SetUp()
    {
        globalProcessIdx = Environment::GetEnv().GetProcessIdx();
        globalProcessCount = Environment::GetEnv().GetProcessCount();
    }
    char* GetErrMessage() { return errMessage; }
    void TearDown() {}

    char errMessage[100];

    BaseTest() { memset(this->errMessage, '\0', 100); }

    virtual int Run(TypedTestParam<T>& param) = 0;

    virtual int Check(TypedTestParam<T>& param) = 0;
};

class MainTest :
    public ::testing::TestWithParam<TestParam>
{
    template <typename T>
    int Run(TestParam param);
    public:
    int Test(TestParam param)
    {
        switch(param.dataType)
        {
            case TDT_FLOAT:
                return Run<float>(param);
            case TDT_DOUBLE:
                return Run<double>(param);
//            case TDT_BYTE:
//                return Run<char>(param);
            default:
                EXPECT_TRUE(false) << "Unknown data type: " << param.dataType;
                return TEST_FAILURE;
        }
    }
};
#endif /* BASE_HPP */
