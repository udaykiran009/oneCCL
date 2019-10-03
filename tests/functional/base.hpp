#ifndef BASE_HPP
#define BASE_HPP

#include <sys/syscall.h>        /* syscall */
#include <sstream>
#include <string>
#include <fstream>
#include "gtest/gtest.h"
#include "ccl.hpp"
#include <cstdlib>
#include <stdlib.h>     /* malloc */

#include <ctime>
using namespace ccl;
using namespace std;


#define sizeofa(arr)        (sizeof(arr) / sizeof(*arr))
#define DTYPE               float
#define CACHELINE_SIZE      64

#define BUFFER_COUNT         7

#define TIMEOUT 30

#define ERR_MESSAGE_MAX_LEN (16384) // TODO: refactor errMessage to avoid fixed sized char array

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
     strToPrint += std::to_string(buf[idx]);         \
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


#define OUTPUT_NAME_ARG "--gtest_output="
#define PATCH_OUTPUT_NAME_ARG(argc, argv)                                                 \
    do                                                                                    \
    {                                                                                     \
        auto comm = ccl::environment::instance().create_communicator();                   \
        if (comm->size() > 1)                                                             \
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
                                + std::to_string(comm->rank())                            \
                                + ".xml";                                                 \
                    PRINT("originArg %s, extPos %zu, argLen %zu, patchedArg %s",          \
                    originArg.c_str(), extPos, argLen, patchedArg.c_str());               \
                    argv[idx][0] = '\0';                                                  \
                    if (comm->rank())                                                     \
                            ::testing::GTEST_FLAG(output) = "";                           \
                        else                                                              \
                            ::testing::GTEST_FLAG(output) = patchedArg.c_str();           \
                }                                                                         \
            }                                                                             \
        }                                                                                 \
    } while (0)

#define SHOW_ALGO(Collective_Name)\
do {   \
    char* algo_name = getenv(Collective_Name);  \
    if (algo_name) \
        printf("%s=%s\n", Collective_Name, algo_name);\
} while (0)

#define SSHOW_ALGO()\
( {   \
    std::string algo_name = getenv(Collective_Name);  \
    std::string transp_name = getenv("CCL_ATL_TRANSPORT");  \
    std::string res_str = algo_name + transp_name; \
    res_str;\
} )

#define RUN_METHOD_DEFINITION(ClassName)                                 \
    template <typename T>                                                \
    int MainTest::Run(TestParam tParam)                                  \
    {                                                                    \
      ClassName<T> className;                                            \
      TypedTestParam<T> typedParam(tParam);                              \
      std::ostringstream output;                                         \
      if (typedParam.processIdx == 0)                                    \
      printf("%s", output.str().c_str());                                \
      int result = className.Run(typedParam);                            \
      int result_final = 0;                                              \
      static int glob_idx = 0;                                           \
      auto comm = ccl::environment::instance().create_communicator();    \
      auto stream = ccl::environment::instance().create_stream();        \
      std::shared_ptr <ccl::request> req;                                \
      ccl::coll_attr coll_attr{};                                        \
      InitCollAttr(&coll_attr);                                          \
      req = comm->allreduce(&result, &result_final, 1,                   \
                         ccl::reduction::sum, &coll_attr, stream); \
      req->wait();                                                       \
      if (result_final > 0)                                              \
      {                                                                  \
          PrintErrMessage(className.GetErrMessage(), output);            \
          if (typedParam.processIdx == 0)                                \
          {                                                              \
              printf("%s", output.str().c_str());                        \
              if (glob_idx) {                                            \
                  TypedTestParam<T> testParam(testParams[glob_idx - 1]); \
                  output << "Previous case:";                            \
                  testParam.Print(output);                               \
              }                                                          \
              output << "Current case:";                                 \
              typedParam.Print(output);                                  \
              EXPECT_TRUE(false) << output.str();                        \
          }                                                              \
          output.str("");                                                \
          output.clear();                                                \
          glob_idx++;                                                    \
          return TEST_FAILURE;                                           \
      }                                                                  \
      glob_idx++;                                                        \
      return TEST_SUCCESS;                                               \
}


#define ASSERT(cond, fmt, ...)                                                       \
    do {                                                                             \
           if (!(cond))                                                              \
           {                                                                         \
                fprintf(stderr, "(%ld): %s:%s:%d: ASSERT '%s' FAILED: " fmt "\n",    \
                GETTID(), __FILE__, __FUNCTION__, __LINE__, #cond, ##__VA_ARGS__);   \
                fflush(stderr);                                                      \
                exit(0);                                                             \
            }                                                                        \
        } while (0)

#define MAIN_FUNCTION()                           \
    int main(int argc, char **argv, char* envs[]) \
    {                                             \
        InitTestParams();                         \
        ccl::environment::instance();             \
        PATCH_OUTPUT_NAME_ARG(argc, argv);        \
        testing::InitGoogleTest(&argc, argv);     \
        int res = RUN_ALL_TESTS();                \
        return res;                               \
    }

#define UNUSED_ATTR __attribute__((unused))

#define TEST_SUCCESS 0
#define TEST_FAILURE 1

#define TEST_CASES_DEFINITION(FuncName)                               \
    TEST_P(MainTest, FuncName) {                                      \
        TestParam param = GetParam();                                 \
        EXPECT_EQ(TEST_SUCCESS, this->Test(param));                   \
    }                                                                 \
    INSTANTIATE_TEST_CASE_P(testParams,                       \
                            MainTest,                                 \
::testing::ValuesIn(testParams));


#define POST_AND_PRE_INCREMENTS(EnumName, LAST_ELEM)                                                                     \
    EnumName& operator++(EnumName& orig) { if (orig != LAST_ELEM) orig = static_cast<EnumName>(orig + 1); return orig; } \
    EnumName operator++(EnumName& orig, int) { EnumName rVal = orig; ++orig; return rVal; }

#define GET_ELEMENT_BEFORE_LAST(EnumName, LAST_ELEM)                                                                     \
    EnumName& operator++(EnumName& orig) { if (orig != LAST_ELEM) orig = static_cast<EnumName>(orig + 1); return orig; } \
    EnumName operator++(EnumName& orig, int) { EnumName rVal = orig; ++orig; return rVal; }

#define SOME_VALUE (0xdeadbeef)


#define ROOT_PROCESS_IDX (0)

typedef enum {
    PT_OOP = 0,
    PT_IN = 1,
    PT_LAST
} PlaceType;
PlaceType firstPlaceType = PT_OOP;
PlaceType lastPlaceType = PT_LAST;
map <int, const char *>placeTypeStr = { {PT_OOP, "PT_OOP"},
                                        {PT_IN, "PT_IN"}
                                        };

typedef enum {
    ST_SMALL = 0,
    ST_MEDIUM = 1,
    ST_LARGE = 2,
    ST_LAST
} SizeType;
SizeType firstSizeType = ST_SMALL;
SizeType lastSizeType = ST_LAST;
map < int, const char *>sizeTypeStr = { {ST_SMALL, "ST_SMALL"},
                                        {ST_MEDIUM, "ST_MEDIUM"},
                                        {ST_LARGE, "ST_LARGE"}
                                        };

map < int, size_t > sizeTypeValues = { {ST_SMALL, 16},
                                       {ST_MEDIUM, 32769},
                                       {ST_LARGE, 524288}
                                       };

typedef enum {
    BC_SMALL = 0,
    BC_MEDIUM = 1,
    BC_LARGE = 2,
    BC_LAST
} BufferCount;
BufferCount firstBufferCount = BC_SMALL;
BufferCount lastBufferCount = BC_LAST;
map < int, const char *>bufferCountStr = { {BC_SMALL, "BC_SMALL"},
                                           {BC_MEDIUM, "BC_MEDIUM"},
                                           {BC_LARGE, "BC_LARGE"}};

map < int, size_t > bufferCountValues = { {BC_SMALL, 1},
                                          {BC_MEDIUM, 2},
                                          {BC_LARGE, 4}};

typedef enum {
    CMPT_WAIT = 0,
    CMPT_TEST = 1,
    CMPT_LAST
} CompletionType;
CompletionType firstCompletionType = CMPT_WAIT;
CompletionType lastCompletionType = CMPT_LAST;
map < int, const char *>completionTypeStr = { {CMPT_WAIT, "CMPT_WAIT"},
                                              {CMPT_TEST, "CMPT_TEST"}};

typedef enum {
#ifdef TEST_CCL_CUSTOM_PROLOG
    PRT_T_TO_2X = 0,
    PRT_T_TO_CHAR = 1,
#endif
    PRT_NULL = 2,
    PRLT_LAST
} PrologType;
PrologType firstPrologType = PRT_NULL;
PrologType lastPrologType = PRLT_LAST;
map < int, const char *>prologTypeStr = { {PRT_NULL, "PRT_NULL"},
#ifdef TEST_CCL_CUSTOM_PROLOG
                                          {PRT_T_TO_2X, "PRT_T_TO_2X"},
                                          {PRT_T_TO_CHAR, "PRT_T_TO_CHAR"}
#endif
                                          };

typedef enum {
#ifdef TEST_CCL_CUSTOM_EPILOG
    EPLT_T_TO_2X = 0,
    EPLT_CHAR_TO_T = 1,
#endif
    EPLT_NULL = 2,
    EPLT_LAST
} EpilogType;
EpilogType firstEpilogType = EPLT_NULL;
EpilogType lastEpilogType = EPLT_LAST;
map < int, const char *>epilogTypeStr = { {EPLT_NULL, "EPLT_NULL"},
#ifdef TEST_CCL_CUSTOM_EPILOG
                                          {EPLT_T_TO_2X, "EPLT_T_TO_2X"},
                                          {EPLT_CHAR_TO_T, "EPLT_CHAR_TO_T"}
#endif
                                          };

typedef enum {
    DT_CHAR = ccl_dtype_char,
    DT_INT = ccl_dtype_int,
    DT_BFP16 = ccl_dtype_bfp16,
    DT_FLOAT = ccl_dtype_float,
    DT_DOUBLE = ccl_dtype_double,
    // DT_INT64 = ccl_dtype_int64,
    // DT_UINT64 = ccl_dtype_uint64,
    DT_LAST
} TestDataType;
TestDataType firstDataType = DT_CHAR;
TestDataType lastDataType = DT_LAST;
map < int, const char *>dataTypeStr = { {DT_CHAR, "DT_CHAR"},
                                        {DT_INT, "DT_INT"},
                                        {DT_BFP16, "DT_BFP16"},
                                        {DT_FLOAT, "DT_FLOAT"},
                                        {DT_DOUBLE, "DT_DOUBLE"}
                                        // {DT_INT64, "INT64"},
                                        // {DT_UINT64, "UINT64"}
};

typedef enum {
    RT_SUM = 0,
#ifdef TEST_CCL_REDUCE
    RT_PROD = 1,
    RT_MIN = 2,
    RT_MAX = 3,
#ifdef TEST_CCL_CUSTOM_REDUCE
    RT_CUSTOM = 4,
    RT_CUSTOM_NULL = 5,
#endif
#endif
    RT_LAST
} TestReductionType;
TestReductionType firstReductionType = RT_SUM;
TestReductionType lastReductionType = RT_LAST;
map < int, const char *>reductionTypeStr = { {RT_SUM, "RT_SUM"},
#ifdef TEST_CCL_REDUCE
                                             {RT_PROD, "RT_PROD"},
                                             {RT_MIN, "RT_MIN"},
                                             {RT_MAX, "RT_MAX"},
#ifdef TEST_CCL_CUSTOM_REDUCE
                                             {RT_CUSTOM, "RT_CUSTOM"},
                                             {RT_CUSTOM_NULL, "RT_CUSTOM_NULL"}
#endif
#endif
                                             };
map < int, ccl_reduction_t > reductionTypeValues = { {RT_SUM, ccl_reduction_sum},
#ifdef TEST_CCL_REDUCE
                                                    {RT_PROD, ccl_reduction_prod},
                                                    {RT_MIN, ccl_reduction_min},
                                                    {RT_MAX, ccl_reduction_max},
#ifdef TEST_CCL_CUSTOM_REDUCE
                                                    {RT_CUSTOM, ccl_reduction_custom},
                                                    {RT_CUSTOM_NULL, ccl_reduction_custom}
#endif
#endif
                                                    };

typedef enum {
    CT_CACHE_0 = 0,
    CT_CACHE_1 = 1,
    CT_LAST
} TestCacheType;
TestCacheType firstCacheType = CT_CACHE_0;
TestCacheType lastCacheType = CT_LAST;
map < int, const char * >cacheTypeStr = { {CT_CACHE_0, "CT_CACHE_0"},
                                          {CT_CACHE_1, "CT_CACHE_1"}
                                          };

map < int, int > cacheTypeValues = { {CT_CACHE_0, 0},
                                     {CT_CACHE_1, 1}
                                     };

typedef enum {
    SNCT_SYNC_0 = 0,
    SNCT_SYNC_1 = 1,
    SNCT_LAST
} TestSyncType;
TestSyncType firstSyncType = SNCT_SYNC_0;
TestSyncType lastSyncType = SNCT_LAST;
map < int, const char * >syncTypeStr = { {SNCT_SYNC_0, "SNCT_SYNC_0"},
                                         {SNCT_SYNC_1, "SNCT_SYNC_1"}};

map < int, int > syncTypeValues = { {SNCT_SYNC_0, 0},
                                    {SNCT_SYNC_1, 1}};


typedef enum {
    PRT_DISABLE = 0,
    PRT_DIRECT = 1,
    PRT_INDIRECT = 2,
    PRT_RANDOM = 3,
    PRT_LAST
} PriorityType;
PriorityType firstPriorityType = PRT_DISABLE;
PriorityType lastPriorityType = PRT_LAST;
map < int, const char * >priorityTypeStr = { {PRT_DISABLE, "PRT_DISABLE"},
                                             {PRT_DIRECT, "PRT_DIRECT"},
                                             {PRT_INDIRECT, "PRT_INDIRECT"},
                                             {PRT_RANDOM, "PRT_RANDOM"}
                                             };

map < int, int > priorityTypeValues = { {PRT_DISABLE, 0},
                                        {PRT_DIRECT, 1},
                                        {PRT_INDIRECT, 2},
                                        {PRT_RANDOM, 3}
};

POST_AND_PRE_INCREMENTS(PlaceType, PT_LAST);
POST_AND_PRE_INCREMENTS(SizeType, ST_LAST);
POST_AND_PRE_INCREMENTS(CompletionType, CMPT_LAST);
POST_AND_PRE_INCREMENTS(TestDataType, DT_LAST);
POST_AND_PRE_INCREMENTS(TestReductionType, RT_LAST);
POST_AND_PRE_INCREMENTS(TestCacheType, CT_LAST);
POST_AND_PRE_INCREMENTS(TestSyncType, SNCT_LAST);
POST_AND_PRE_INCREMENTS(PriorityType, PRT_LAST);
POST_AND_PRE_INCREMENTS(BufferCount, BC_LAST);
POST_AND_PRE_INCREMENTS(PrologType, PRLT_LAST);
POST_AND_PRE_INCREMENTS(EpilogType, EPLT_LAST);

void InitCollAttr(ccl::coll_attr* coll_attr)
{
    coll_attr->prologue_fn = NULL;
    coll_attr->epilogue_fn = NULL;
    coll_attr->reduction_fn = NULL;
    coll_attr->priority = 0;
    coll_attr->synchronous = 0;
    coll_attr->match_id = NULL;
    coll_attr->to_cache = 0;
}

void PrintErrMessage(char* errMessage, std::ostream &output)
{
    int messageLen = strlen(errMessage);
    auto comm = ccl::environment::instance().create_communicator();
    auto stream = ccl::environment::instance().create_stream();
    std::shared_ptr <ccl::request> req;
    ccl::coll_attr coll_attr{};
    InitCollAttr(&coll_attr);
    int processCount = comm->size();
    int processIdx = comm->rank();
    size_t* arrMessageLen = new size_t[processCount];
    int* arrMessageLen_copy = new int[processCount];
    size_t* displs = new size_t[processCount];
    displs[0] = 1;
    for (int i = 1; i < processCount; i++)
        displs[i] = 1;
    req = comm->allgatherv(&messageLen, 1, arrMessageLen_copy, displs, &coll_attr, stream);
    req->wait();
    for (int i = 0; i < processCount; i++)
        arrMessageLen[i] = arrMessageLen_copy[i];
    int fullMessageLen = 0;
    for (int i = 0; i < processCount; i++)
        fullMessageLen += arrMessageLen[i];
    if (fullMessageLen == 0)
    {
        delete[] arrMessageLen;
        delete[] displs;
        return;
    }
    char* arrErrMessage = new char[fullMessageLen];
    req = comm->allgatherv(errMessage, messageLen, arrErrMessage, arrMessageLen, &coll_attr, stream);
    req->wait();
    if (processIdx == 0)
    {
        output << arrErrMessage;
    }
    delete[] arrMessageLen;
    delete[] arrMessageLen_copy;
    delete[] arrErrMessage;
    delete[] displs;

}

//This struct needed for gtest
struct TestParam {
    PlaceType placeType;
    TestCacheType cacheType;
    TestSyncType syncType;
    SizeType sizeType;
    CompletionType completionType;
    TestReductionType reductionType;
    TestDataType dataType;
    PriorityType priorityType;
    PriorityType priorityStartType;
    BufferCount bufferCount;
    PrologType prologType;
    EpilogType epilogType;
};

size_t CalculateTestCount ()
{
    size_t testCount = PRT_LAST * PRT_LAST * CMPT_LAST * SNCT_LAST * (DT_LAST-1) * ST_LAST *  RT_LAST * BC_LAST * CT_LAST * PT_LAST * PRLT_LAST * EPLT_LAST;
// CCL_TEST_EPILOG_TYPE=0 CCL_TEST_PROLOG_TYPE=0 CCL_TEST_PLACE_TYPE=0 CCL_TEST_CACHE_TYPE=0 CCL_TEST_BUFFER_COUNT=0 CCL_TEST_SIZE_TYPE=0 CCL_TEST_PRIORITY_TYPE=1 CCL_TEST_COMPLETION_TYPE=0 CCL_TEST_SYNC_TYPE=0 CCL_TEST_REDUCTION_TYPE=0 CCL_TEST_DATA_TYPE=0
    char* testDataTypeEnabled = getenv("CCL_TEST_DATA_TYPE");
    char* testReductionEnabled = getenv("CCL_TEST_REDUCTION_TYPE");
    char* testSyncEnabled = getenv("CCL_TEST_SYNC_TYPE");
    char* testCompletionEnabled = getenv("CCL_TEST_COMPLETION_TYPE");
    char* testPriorityEnabled = getenv("CCL_TEST_PRIORITY_TYPE");
    char* testSizeTypeEnabled = getenv("CCL_TEST_SIZE_TYPE");
    char* testBufferCountEnabled = getenv("CCL_TEST_BUFFER_COUNT");
    char* testCacheEnabled = getenv("CCL_TEST_CACHE_TYPE");
    char* testPlaceTypeEnabled = getenv("CCL_TEST_PLACE_TYPE");
    char* testPrologEnabled = getenv("CCL_TEST_PROLOG_TYPE");
    char* testEpilogEnabled = getenv("CCL_TEST_EPILOG_TYPE");
    if (testDataTypeEnabled && atoi(testDataTypeEnabled) == 0) {
        testCount /= (lastDataType - 1);
        firstDataType = static_cast<TestDataType>(DT_FLOAT);
        lastDataType = static_cast<TestDataType>(firstDataType + 1);
        }
    if (testReductionEnabled && atoi(testReductionEnabled) == 0) {
        testCount /= lastReductionType;
        firstReductionType = static_cast<TestReductionType>(RT_SUM);
        lastReductionType = static_cast<TestReductionType>(firstReductionType + 1);
        }
    if (testSyncEnabled && atoi(testSyncEnabled) == 0) {
        testCount /= lastSyncType;
        firstSyncType = static_cast<TestSyncType>(SNCT_SYNC_1);
        lastSyncType = static_cast<TestSyncType>(firstSyncType + 1);
        }
    if (testCompletionEnabled && atoi(testCompletionEnabled) == 0) {
        testCount /= lastCompletionType;
        firstCompletionType = static_cast<CompletionType>(CMPT_WAIT);
        lastCompletionType = static_cast<CompletionType>(firstCompletionType + 1);
        }
    if (testPriorityEnabled && atoi(testPriorityEnabled) == 0) {
        testCount /= (lastPriorityType * lastPriorityType);
        firstPriorityType = static_cast<PriorityType>(PRT_DISABLE);
        lastPriorityType = static_cast<PriorityType>(firstPriorityType + 1);
        }
    if (testSizeTypeEnabled && atoi(testSizeTypeEnabled) == 0) {
        testCount /= lastSizeType;
        firstSizeType = static_cast<SizeType>(ST_MEDIUM);
        lastSizeType = static_cast<SizeType>(firstSizeType + 1);
        }
    if (testBufferCountEnabled && atoi(testBufferCountEnabled) == 0) {
        testCount /= lastBufferCount;
        firstBufferCount = static_cast<BufferCount>(BC_MEDIUM);
        lastBufferCount = static_cast<BufferCount>(firstBufferCount + 1);
        }
    if (testCacheEnabled && atoi(testCacheEnabled) == 0) {
        testCount /= lastCacheType;
        firstCacheType = static_cast<TestCacheType>(CT_CACHE_1);
        lastCacheType = static_cast<TestCacheType>(firstCacheType + 1);
        }
    if (testPlaceTypeEnabled && atoi(testPlaceTypeEnabled) == 0) {
        testCount /= lastPlaceType;
        firstPlaceType = static_cast<PlaceType>(PT_IN);
        lastPlaceType = static_cast<PlaceType>(firstPlaceType + 1);
        }
    if (testPrologEnabled && atoi(testPrologEnabled) == 0) {
        testCount /= lastPrologType;
        firstPrologType = static_cast<PrologType>(PRT_NULL);
        lastPrologType = static_cast<PrologType>(firstPrologType + 1);
        }
    if (testEpilogEnabled && atoi(testEpilogEnabled) == 0) {
        testCount /= lastEpilogType;
        firstEpilogType = static_cast<EpilogType>(EPLT_NULL);
        lastEpilogType = static_cast<EpilogType>(firstEpilogType + 1);
        }
    return testCount;
}

std::vector<TestParam> testParams(CalculateTestCount());

void InitTestParams()
{
    size_t idx = 0;
    for (PrologType prologType = firstPrologType; prologType < lastPrologType; prologType++) {
        for (EpilogType epilogType = firstEpilogType; epilogType < lastEpilogType; epilogType++) {
            for (TestReductionType reductionType = firstReductionType; reductionType < lastReductionType; reductionType++) {
                for (TestSyncType syncType = firstSyncType; syncType < lastSyncType; syncType++) {
                    for (TestCacheType cacheType = firstCacheType; cacheType < lastCacheType; cacheType++) {
                        for (SizeType sizeType = firstSizeType; sizeType < lastSizeType; sizeType++) {
                            for (TestDataType dataType = firstDataType; dataType < lastDataType; dataType++) {
                                if (dataType == DT_BFP16)
                                    // TODO: remove skipped data type
                                    continue;
                                for (CompletionType completionType = firstCompletionType; completionType < lastCompletionType; completionType++) {
                                    for (PlaceType placeType = firstPlaceType; placeType < lastPlaceType; placeType++) {
                                        for (PriorityType priorityType = firstPriorityType; priorityType < lastPriorityType; priorityType++) {
                                            for (PriorityType priorityStartType = firstPriorityType; priorityStartType < lastPriorityType; priorityStartType++) {
                                                for (BufferCount bufferCount = firstBufferCount; bufferCount < lastBufferCount; bufferCount++) {
                                                    testParams[idx].placeType = placeType;
                                                    testParams[idx].sizeType = sizeType;
                                                    testParams[idx].dataType = dataType;
                                                    testParams[idx].cacheType = cacheType;
                                                    testParams[idx].syncType = syncType;
                                                    testParams[idx].completionType = completionType;
                                                    testParams[idx].reductionType = reductionType;
                                                    testParams[idx].bufferCount = bufferCount;
                                                    testParams[idx].priorityType = priorityType;
                                                    testParams[idx].priorityStartType = priorityStartType;
                                                    testParams[idx].prologType = prologType;
                                                    testParams[idx].epilogType = epilogType;
                                                    idx++;
                                                }
                                            }
                                        }
                                    }
                                }
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
    size_t elemCount;
    size_t bufferCount;
    size_t processCount;
    size_t processIdx;
    std::vector<std::vector<T>> sendBuf;
    std::vector<std::vector<T>> recvBuf;
    ccl::communicator_t comm;
    std::vector<std::shared_ptr<ccl::request>> req;
    ccl::coll_attr coll_attr{};
    std::string match_id;
    ccl::communicator_t global_comm;
    ccl::stream_t stream;
    size_t *startArr;

    TypedTestParam(TestParam tParam):testParam(tParam) {
        InitCollAttr(&coll_attr);
        elemCount = GetElemCount();
        bufferCount = GetBufferCount();
        comm = ccl::environment::instance().create_communicator();
        global_comm = ccl::environment::instance().create_communicator();
        processCount = comm->size();
        processIdx = comm->rank();
        sendBuf.resize(bufferCount);
        recvBuf.resize(bufferCount);
        startArr = (size_t*) malloc(bufferCount * sizeof(size_t));
        req.resize(bufferCount);
        for (size_t i = 0; i < bufferCount; i++)
            sendBuf[i].resize(elemCount * processCount * sizeof(T));
    }

    std::string CreateMatchId(size_t idx)
    {
        return (std::to_string(idx) +
                std::to_string(processCount) +
                std::to_string(elemCount) +
                std::to_string(GetReductionType()) +
                std::to_string(GetSyncType()) +
                std::to_string(GetCacheType()) +
                std::to_string(GetSizeType()) +
                std::to_string(GetDataType()) +
                std::to_string(GetCompletionType()) +
                std::to_string(GetPlaceType()) +
                std::to_string(GetPriorityStartType()) +
                std::to_string(GetPriorityType()) +
                std::to_string(bufferCount) +
                std::to_string(GetPrologType()) +
                std::to_string(GetEpilogType()));
    }

    bool CompleteRequest(std::shared_ptr < ccl::request > req)
    {
        if (testParam.completionType == CMPT_TEST)
        {
            return req->test();
        }
        else if (testParam.completionType == CMPT_WAIT)
        {
            req->wait();
            return true;
        }
        else
        {
            ASSERT(0, "unexpected completion type %d", testParam.completionType);
            return false;
        }
    }

    size_t* DefineStartOrder()
    {
            size_t idx = 0;
                if (testParam.priorityStartType == PRT_DIRECT || testParam.priorityStartType == PRT_DISABLE)
                    for (idx = 0; idx < bufferCount; idx++)
                        startArr[idx] = idx;
                else if (testParam.priorityStartType == PRT_INDIRECT)
                    for (idx = 0; idx < bufferCount; idx++)
                        startArr[idx] = (bufferCount - idx - 1);
                // TODO: should be enabled after CCL_UNORDERED_COLL
                else if (testParam.priorityStartType == PRT_RANDOM){
                    char* testUnorderedColl = getenv("CCL_UNORDERED_COLL");
                    if (testUnorderedColl && atoi(testUnorderedColl) == 1) {
                        //size_t j;
                        for (idx = 0; idx < bufferCount; idx++)
                            startArr[idx] = idx;
                        // for(int k=bufferCount; k>1; k--) {
                        //    j = (rand() + processIdx) % k;
                        //    int tmp = startArr[k-1];
                        //    startArr[k-1] = startArr[j];
                        //    startArr[j] = tmp;
                        // }
                    }
                    else {
                    for (idx = 0; idx < bufferCount; idx++)
                         startArr[idx] = idx;
                    }
                }
                else
                    startArr[idx] = idx;
        return startArr;
    }
    bool DefineCompletionOrderAndComplete()
    {
            size_t idx, msg_idx;
            size_t completions = 0;
            int msg_completions[bufferCount];

            memset(msg_completions, 0, bufferCount * sizeof(int));

            while (completions < bufferCount)
            {
                for (idx = 0; idx < bufferCount; idx++)
                {
                    if (testParam.priorityType == PRT_DIRECT || testParam.priorityType == PRT_DISABLE)
                    {
                        msg_idx = idx;
                    }
                    else if (testParam.priorityType == PRT_INDIRECT)
                    {
                        msg_idx = (bufferCount - idx - 1);
                    }
                    else if (testParam.priorityType == PRT_RANDOM)
                    {
                        msg_idx = rand() % bufferCount;
                    }
                    else
                        msg_idx = idx;

                    if (msg_completions[msg_idx]) continue;

                    if (CompleteRequest(req[msg_idx]))
                    {
                        completions++;
                        msg_completions[msg_idx] = 1;
                    }
                }
            }
        return TEST_SUCCESS;
    }

    size_t CreatePriorityValue(size_t idx)
    {
        return (testParam.priorityType != PRT_DISABLE) ? idx : 0;
    }

    void Print(std::ostream &output) {
        char strParameters[1000];
        memset(strParameters, '\0', 1000);
        sprintf(strParameters, "\nTest params: \
               \nprocessCount %zu \
               \nprocessIdx %zu \
               \nelemCount %zu \
               \nreductionType %s \
               \nsyncType %s \
               \ncacheType %s \
               \nsizeValue %s \
               \ndataType %s \
               \ncompletionType %s \
               \nplaceType %s \
               \npriorityStartType %s \
               \npriorityType %s \
               \nbufferCount %zu \
               \nprologType %s \
               \nepilogType %s \
               \nmatch_id %s \
               \n-------------\n",
               processCount,
               processIdx,
               elemCount,
               GetReductionTypeStr(),
               GetSyncTypeStr(),
               GetCacheTypeStr(),
               GetSizeTypeStr(),
               GetDataTypeStr(),
               GetCompletionTypeStr(),
               GetPlaceTypeStr(),
               GetPriorityStartTypeStr(),
               GetPriorityTypeStr(),
               bufferCount,
               GetPrologTypeStr(),
               GetEpilogTypeStr(),
               match_id.c_str()
               );
       output << strParameters;
       fflush(stdout);
    }

    ccl::stream_t& GetStream() {
        return stream;
    }
    const char *GetPlaceTypeStr() {
        return placeTypeStr[testParam.placeType];
    }
    int GetCacheType() {
        return cacheTypeValues[testParam.cacheType];
    }
    int GetSyncType() {
        return syncTypeValues[testParam.syncType];
    }
    const char *GetSizeTypeStr() {
        return sizeTypeStr[testParam.sizeType];
    }
    const char *GetDataTypeStr() {
        return dataTypeStr[testParam.dataType];
    }
    const char *GetReductionTypeStr() {
        return reductionTypeStr[testParam.reductionType];
    }
    const char *GetCompletionTypeStr() {
        return completionTypeStr[testParam.completionType];
    }
    const char *GetPriorityTypeStr() {
        return priorityTypeStr[testParam.priorityType];
    }
    const char *GetPriorityStartTypeStr() {
        return priorityTypeStr[testParam.priorityStartType];
    }
    const char *GetCacheTypeStr() {
        return cacheTypeStr[testParam.cacheType];
    }
    const char *GetSyncTypeStr() {
        return syncTypeStr[testParam.syncType];
    }
    size_t GetElemCount() {
        return sizeTypeValues[testParam.sizeType];
    }
    size_t GetBufferCount() {
        return bufferCountValues[testParam.bufferCount];
    }
    CompletionType GetCompletionType() {
        return testParam.completionType;
    }
    PriorityType GetPriorityStartType() {
        return testParam.priorityStartType;
    }
    PlaceType GetPlaceType() {
        return testParam.placeType;
    }
    SizeType GetSizeType() {
        return testParam.sizeType;
    }
    TestDataType GetDataType() {
        return testParam.dataType;
    }
    TestReductionType GetReductionType() {
        return testParam.reductionType;
    }
    ccl_reduction_t GetCCLReductionType() {
        return reductionTypeValues[testParam.reductionType];
    }
    PriorityType GetPriorityType() {
        return testParam.priorityType;
    }
    PrologType GetPrologType() {
        return testParam.prologType;
    }
    EpilogType GetEpilogType() {
        return testParam.epilogType;
    }
    const char *GetPrologTypeStr() {
        return prologTypeStr[testParam.prologType];
    }
    const char *GetEpilogTypeStr() {
        return epilogTypeStr[testParam.epilogType];
    }
};

template <typename T>
T get_expected_min(size_t i, size_t j, size_t processCount, size_t coeff = 1)
{
    if ((T)(coeff *(i + j + processCount - 1)) < T(coeff * (i + j)))
        return (T)(coeff * (i + j + processCount - 1));
    return (T)(coeff * (i + j));
}
template <typename T>
T get_expected_max(size_t i, size_t j, size_t processCount, size_t coeff = 1)
{
    if ((T)(coeff *(i + j + processCount - 1)) > T(coeff * (i + j)))
        return (T)(coeff * (i + j + processCount - 1));
    return (T)(coeff * (i + j));
}

template <typename T> class BaseTest {

public:
    size_t globalProcessIdx;
    size_t globalProcessCount;
    ccl::communicator_t comm;

    void SetUp() {
        globalProcessIdx = comm->rank();
        globalProcessCount = comm->size();
    }
    char *GetErrMessage() {
        return errMessage;
    }
    void TearDown() {
    }

    char errMessage[ERR_MESSAGE_MAX_LEN]{};

    BaseTest()
    {
        comm = ccl::environment::instance().create_communicator();
        memset(this->errMessage, '\0', ERR_MESSAGE_MAX_LEN);
    }
    void Init(TypedTestParam <T> &param, size_t idx){
        param.coll_attr.priority = param.CreatePriorityValue(idx);
        param.coll_attr.to_cache = (int)param.GetCacheType();
        char* testUnorderedColl = getenv("CCL_UNORDERED_COLL");
        if (testUnorderedColl && atoi(testUnorderedColl) == 1)
            param.coll_attr.synchronous = 0;
        else
            param.coll_attr.synchronous = (int)param.GetSyncType();
        param.match_id = param.CreateMatchId(idx);
        param.coll_attr.match_id = param.match_id.c_str();
    }
    void SwapBuffers(TypedTestParam <T> &param, size_t iter){
        char* testDynamicPointer = getenv("CCL_TEST_DYNAMIC_POINTER");
        if (testDynamicPointer && atoi(testDynamicPointer) == 1) {
            if (iter == 1) {
                if (param.processIdx % 2 ) {
                    std::vector<std::vector<T>> tmpBuf;
                    tmpBuf.resize(param.bufferCount);
                    for (size_t i = 0; i < param.bufferCount; i++)
                        tmpBuf[i].resize(param.elemCount * param.processCount * sizeof(T));
                    for (size_t j = 0; j < param.bufferCount; j++) {
                        for (size_t i = 0; i < param.elemCount; i++) {
                            tmpBuf[j][i] = param.sendBuf[j][i];
                        }
                    }
                    param.sendBuf.swap(tmpBuf);
                }
            }
        }
    }
    void FillBuffers(TypedTestParam <T> &param){
        for (size_t j = 0; j < param.bufferCount; j++) {
            for (size_t i = 0; i < param.elemCount; i++) {
                param.sendBuf[j][i] = param.processIdx + i + j;
            }
        }
        if (param.testParam.placeType == PT_OOP)
        {
            for (size_t i = 0; i < param.bufferCount; i++)
                param.recvBuf[i].resize(param.elemCount * param.processCount * sizeof(T));
        }
        else
        {
            for (size_t i = 0; i < param.bufferCount; i++)
            {
                    param.recvBuf[i] = param.sendBuf[i];
            }
        }
    }
    virtual int Run(TypedTestParam <T> &param) = 0;
    virtual int Check(TypedTestParam <T> &param) = 0;

};

class MainTest : public::testing :: TestWithParam <TestParam> {
    template <typename T>
    int Run(TestParam param);
public:
    int Test(TestParam param) {
        switch (param.dataType) {
        case DT_CHAR:
            return Run <char>(param);
        case DT_INT:
            return Run <int>(param);
        //TODO: add additional type to testing
        // case DT_BFP16:
           // return Run <>(param);
        case DT_FLOAT:
            return Run <float>(param);
        case DT_DOUBLE:
            return Run <double>(param);
        // case DT_INT64:
            // return TEST_SUCCESS;
        // case DT_UINT64:
            // return TEST_SUCCESS;
        default:
            EXPECT_TRUE(false) << "Unknown data type: " << param.dataType;
            return TEST_FAILURE;
        }
    }
};
#endif /* BASE_HPP */
