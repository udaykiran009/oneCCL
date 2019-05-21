#ifndef BASE_HPP
#define BASE_HPP

#include <sys/syscall.h>        /* syscall */
#include <sstream>
#include <string>
#include <fstream>
#include "gtest/gtest.h"
#include "mlsl.hpp"
#include <cstdlib>      /* malloc */

#include <ctime>
using namespace mlsl;
using namespace std;


#define sizeofa(arr)        (sizeof(arr) / sizeof(*arr))
#define DTYPE               float
#define CACHELINE_SIZE      64

#define BUFFER_COUNT         7

#define TIMEOUT 30

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
        mlsl::communicator comm;                                                          \
        if (comm.size() > 1)                                                              \
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
                                + std::to_string(comm.rank())                             \
                                + ".xml";                                                 \
                    PRINT("originArg %s, extPos %zu, argLen %zu, patchedArg %s",          \
                    originArg.c_str(), extPos, argLen, patchedArg.c_str());               \
                    argv[idx] = '\0';                                                     \
                    if (comm.rank())                                                      \
                            ::testing::GTEST_FLAG(output) = "";                           \
                        else                                                              \
                            ::testing::GTEST_FLAG(output) = patchedArg.c_str();           \
                }                                                                         \
            }                                                                             \
        }                                                                                 \
    } while (0)


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
      mlsl::communicator comm;                                           \
      std::shared_ptr <mlsl::request> req;                               \
      req = comm.allreduce(&result, &result_final, 1,                    \
                           mlsl::data_type::dtype_int,                   \
                           mlsl::reduction::sum);                        \
      req->wait();                                                       \
      if (result_final > 0)                                              \
      {                                                                  \
          PrintErrMessage(className.GetErrMessage(), output);            \
          if (typedParam.processIdx == 0)                                \
          {                                                              \
              printf("%s", output.str().c_str());                        \
              if (glob_idx) {                                            \
                  TypedTestParam<T> testParam(testParams[glob_idx - 1]); \
                  testParam.Print(output);                               \
              }                                                          \
              typedParam.Print(output);                                  \
              EXPECT_TRUE(false) << output.str();                        \
          }                                                              \
          output.str("");                                                \
          output.clear();                                                \
          return TEST_FAILURE;                                           \
      }         glob_idx++;                                              \
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
    int main(int argc, char **argv)               \
    {                                             \
        InitTestParams();                         \
        mlsl::environment env;                    \
        PATCH_OUTPUT_NAME_ARG(argc, argv);        \
        testing::InitGoogleTest(&argc, argv);     \
        int res = RUN_ALL_TESTS();                \
        env.~environment();                       \
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
    INSTANTIATE_TEST_CASE_P(TestWithParameters,                       \
                            MainTest,                                 \
::testing::ValuesIn(testParams));

#define POST_AND_PRE_INCREMENTS(EnumName, LAST_ELEM)                                                                     \
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
map <int, const char *>placeTypeStr = { {PT_OOP, "PT_OOP"},
                                        {PT_IN, "PT_IN"}};

typedef enum {
    ST_SMALL = 0,
    ST_MEDIUM = 1,
    ST_LARGE = 2,
    ST_LAST
} SizeType;
SizeType firstSizeType = ST_SMALL;
map < int, const char *>sizeTypeStr = { {ST_SMALL, "ST_SMALL"},
                                        {ST_MEDIUM, "ST_MEDIUM"},
                                        {ST_LARGE, "ST_LARGE"}};

map < int, size_t > sizeTypeValues = { {ST_SMALL, 16},
                                       {ST_MEDIUM, 32768},
                                       {ST_LARGE, 524288}};

typedef enum {
    BC_SMALL = 0,
    BC_MEDIUM = 1,
    BC_LARGE = 2,
    BC_LAST
} BufferCount;
BufferCount firstBufferCount = BC_SMALL;
map < int, const char *>bufferCountStr = { {BC_SMALL, "BC_SMALL"},
                                           {BC_MEDIUM, "BC_MEDIUM"},
                                           {BC_LARGE, "BC_LARGE"}};

map < int, size_t > bufferCountValues = { {BC_SMALL, 1},
                                          {BC_MEDIUM, 7},
                                          {BC_LARGE, 12}};

typedef enum {
    CMPT_WAIT = 0,
    CMPT_TEST = 1,
    CMPT_LAST
} CompletionType;
CompletionType firstCompletionType = CMPT_WAIT;
map < int, const char *>completionTypeStr = { {CMPT_WAIT, "CMPT_WAIT"},
                                              {CMPT_TEST, "CMPT_TEST"}};

typedef enum {
    DT_CHAR = mlsl_dtype_char,
    DT_INT = mlsl_dtype_int,
    DT_BFP16 = mlsl_dtype_bfp16,
    DT_FLOAT = mlsl_dtype_float,
    DT_DOUBLE = mlsl_dtype_double,
    // DT_INT64 = mlsl_dtype_int64,
    // DT_UINT64 = mlsl_dtype_uint64,
    DT_LAST
} TestDataType;
TestDataType firstDataType = DT_CHAR;
map < int, const char *>dataTypeStr = { {DT_CHAR, "DT_CHAR"},
                                        {DT_INT, "DT_INT"},
                                        {DT_BFP16, "DT_BFP16"},
                                        {DT_FLOAT, "DT_FLOAT"},
                                        {DT_DOUBLE, "DT_DOUBLE"}
                                        // {DT_INT64, "INT64"},
                                        // {DT_UINT64, "UINT64"}
};

typedef enum {
    RT_SUM = mlsl_reduction_sum,
#ifdef TEST_MLSL_REDUCE
    RT_PROD = mlsl_reduction_prod,
    RT_MIN = mlsl_reduction_min,
    RT_MAX = mlsl_reduction_max,
#endif
    RT_LAST
} TestReductionType;
TestReductionType firstReductionType = RT_SUM;
map < int, const char *>reductionTypeStr = { {RT_SUM, "RT_SUM"},
#ifdef TEST_MLSL_REDUCE
                                             {RT_PROD, "RT_PROD"},
                                             {RT_MIN, "RT_MIN"},
                                             {RT_MAX, "RT_MAX"}
#endif
                                             };

typedef enum {
    CT_CACHE_0 = 0,
//    CT_CACHE_1 = 1,
    CT_LAST
} TestCacheType;
TestCacheType firstCacheType = CT_CACHE_0;
map < int, const char * >cacheTypeStr = { {CT_CACHE_0, "CT_CACHE_0"},
//                                          {CT_CACHE_1, "CT_CACHE_1"}
                                          };

map < int, int > cacheTypeValues = { {CT_CACHE_0, 0},
//                                     {CT_CACHE_1, 1}
                                     };

typedef enum {
    SNCT_SYNC_0 = 0,
    SNCT_SYNC_1 = 1,
    SNCT_LAST
} TestSyncType;
TestSyncType firstSyncType = SNCT_SYNC_0;
map < int, const char * >syncTypeStr = { {SNCT_SYNC_0, "SNCT_SYNC_0"},
                                         {SNCT_SYNC_1, "SNCT_SYNC_1"}};
                                                      
map < int, int > syncTypeValues = { {SNCT_SYNC_0, 0},
                                    {SNCT_SYNC_1, 1}};


typedef enum {
    PRT_DISABLE = 0,
    PRT_ENABLE = 1,
    PRT_LAST
} PriorityType;
PriorityType firstPriorityType = PRT_DISABLE;
map < int, const char * >priorityTypeStr = { {PRT_DISABLE, "PRT_DISABLE"},
                                             {PRT_ENABLE, "PRT_ENABLE"}};

map < int, int > priorityTypeValues = { {PRT_DISABLE, 0},
                                        {PRT_ENABLE, 1}
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

void InitCollAttr(mlsl_coll_attr_t *coll_attr)
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
    size_t messageLen = strlen(errMessage);
    mlsl::communicator comm;
    std::shared_ptr <mlsl::request> req;
    mlsl_coll_attr_t coll_attr {};
    InitCollAttr(&coll_attr);
    int processCount = comm.size();
    int processIdx = comm.rank();
    size_t* arrMessageLen = new size_t[processCount];
    int* arrMessageLen_copy = new int[processCount];
    size_t* displs = new size_t[processCount];
    displs[0] = 1;
    for (int i = 1; i < processCount; i++)
        displs[i] = 1;
    req = comm.allgatherv(&messageLen, 1, arrMessageLen_copy, displs, mlsl::data_type::dtype_int, &coll_attr);
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
    req = comm.allgatherv(errMessage, messageLen, arrErrMessage, arrMessageLen, mlsl::data_type::dtype_char, &coll_attr);
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
    BufferCount bufferCount;
};

#define TEST_COUNT (PRT_LAST * CMPT_LAST * SNCT_LAST * DT_LAST * ST_LAST *  RT_LAST * BC_LAST * CT_LAST * PT_LAST)
std::vector<TestParam> testParams(TEST_COUNT);

void InitTestParams()
{
    size_t idx = 0;
    for (TestReductionType reductionType = firstReductionType; reductionType < RT_LAST; reductionType++) {
        for (TestSyncType syncType = firstSyncType; syncType < SNCT_LAST; syncType++) {
            for (TestCacheType cacheType = firstCacheType; cacheType < CT_LAST; cacheType++) {
                for (SizeType sizeType = firstSizeType; sizeType < ST_LAST; sizeType++) {
                    for (TestDataType dataType = firstDataType; dataType < DT_LAST; dataType++) {
                         for (CompletionType completionType = firstCompletionType; completionType < CMPT_LAST; completionType++) {
                            for (PlaceType placeType = firstPlaceType; placeType < PT_LAST; placeType++) {
                                for (PriorityType priorityType = firstPriorityType; priorityType < PRT_LAST; priorityType++) {
                                    for (BufferCount bufferCount = firstBufferCount; bufferCount < BC_LAST; bufferCount++) {
                                        testParams[idx].placeType = placeType;
                                        testParams[idx].sizeType = sizeType;
                                        testParams[idx].dataType = dataType;
                                        testParams[idx].cacheType = cacheType;
                                        testParams[idx].syncType = syncType;
                                        testParams[idx].completionType = completionType;
                                        testParams[idx].reductionType = reductionType;
                                        testParams[idx].bufferCount = bufferCount;
                                        testParams[idx].priorityType = priorityType;
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
    mlsl::communicator comm;
    std::vector<std::shared_ptr<mlsl::request>> req;
    mlsl_coll_attr_t coll_attr {};
    mlsl::communicator global_comm;

    TypedTestParam(TestParam tParam):testParam(tParam) {
        InitCollAttr(&coll_attr);
        elemCount = GetElemCount();
        bufferCount = GetBufferCount();
        processCount = comm.size();
        processIdx = comm.rank();
        sendBuf.resize(bufferCount); 
        recvBuf.resize(bufferCount); 
        req.resize(bufferCount);
        for (size_t i = 0; i < bufferCount; i++)
            sendBuf[i].resize(elemCount * processCount * sizeof(T));
        if (testParam.placeType == PT_OOP)
            for (size_t i = 0; i < bufferCount; i++)
                recvBuf[i].resize(elemCount * processCount * sizeof(T));
        else
            for (size_t i = 0; i < bufferCount; i++)
                recvBuf[i] = sendBuf[i];
    }

    void CompleteRequest(std::shared_ptr < mlsl::request > req) { 
        if (testParam.completionType == CMPT_TEST) {
            bool isCompleted = false;
            size_t count = 0;
            do {
                isCompleted = req->test();
                count++;
            } while (!isCompleted);
        }
        else if (testParam.completionType == CMPT_WAIT)
            req->wait();
        else
            ASSERT(0, "unexpected completion type %d", testParam.completionType);
    }

    size_t PriorityRequest() {
        size_t min_priority = 0, max_priority = 0, priority, idx = 0, comp_iter_time_ms = 0, comp_delay_ms;
        if (testParam.priorityType == PRT_ENABLE) {
            size_t msg_completions[bufferCount];
            char* comp_iter_time_ms_env = getenv("COMP_ITER_TIME_MS");
            if (comp_iter_time_ms_env)
            {
                comp_iter_time_ms = atoi(comp_iter_time_ms_env);
            }
            comp_delay_ms = 2 * comp_iter_time_ms /bufferCount;
            memset(msg_completions, 0, bufferCount * sizeof(size_t));
            for (idx = 0; idx < bufferCount; idx++) {   
                if (idx % 2 == 0) usleep(comp_delay_ms * 1000);
                priority = min_priority + idx;
                if (priority > max_priority) 
                    priority = max_priority;
            }
        }
        else
            priority = 0;
        return priority; 
    }
	
void Print(std::ostream &output) {
            char strParameters[1000];
        memset(strParameters, '\0', 1000);
        sprintf(strParameters, "\nTest params: \
               \nprocessIdx %zu \
               \nbufferCount %zu \
               \nreductionType %s \
               \nplaceType %s \
               \ncacheType %s \
               \nsizeValue %s \
               \npriorityType %s \
               \nsyncType %s \
               \ndataType %s \
               \ncompletionType %s \
               \nelemCount %zu \
               \nprocessCount %zu \
               \n-------------\n",
               processIdx,
               bufferCount,
               GetReductionTypeStr(),
               GetPlaceTypeStr(),
               GetCacheTypeStr(),
               GetSizeTypeStr(),
               GetPriorityTypeStr(),
               GetSyncTypeStr(),
               GetDataTypeStr(),
               GetCompletionTypeStr(),
               elemCount,
               processCount
               );
               output << strParameters;
               fflush(stdout);
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
    PriorityType GetPriorityType() {
        return testParam.priorityType;
    }
friend std::ostream&  operator<<(std::ostream & stream, const TestParam & tParam);  
};
std::ostream&  operator<<(std::ostream & stream, const TestParam & tParam) {
    mlsl::communicator comm;
    size_t processIdx = comm.rank();
    return stream 
    << " \nprocessIdx " << processIdx
    << " \nbufferCount " << bufferCountStr[tParam.bufferCount]
    << " \nreductionType " << reductionTypeStr[tParam.reductionType]
    << " \nplaceType " << placeTypeStr[tParam.placeType]
    << " \ncacheType " << cacheTypeStr[tParam.cacheType]
    << " \nsizeType " << sizeTypeStr[tParam.sizeType]
    << " \npriorityType " << priorityTypeStr[tParam.priorityType]
    << " \nsyncType " << syncTypeStr[tParam.syncType]
    << " \ndataType " << dataTypeStr[tParam.dataType]
    << " \ndataType " << completionTypeStr[tParam.completionType];
}

template <typename T> class BaseTest {

public:
    size_t globalProcessIdx;
    size_t globalProcessCount;
    mlsl::communicator comm;

    void SetUp() {
        globalProcessIdx = comm.rank();
        globalProcessCount = comm.size();
    }
    char *GetErrMessage() {
        return errMessage;
    }
    void TearDown() {
    }

    char errMessage[100]{};

    BaseTest() { memset(this->errMessage, '\0', 100); }
    virtual void Init(TypedTestParam <T> &param){
            param.coll_attr.priority = (int)param.PriorityRequest();
            param.coll_attr.to_cache = (int)param.GetCacheType();
            param.coll_attr.synchronous = (int)param.GetSyncType();
    }
    T get_expected_min(size_t i, size_t processCount)
    {
        if ((T)(i + processCount - 1) < T(i))
            return (T)(i + processCount - 1);
        return (T)i;
    }

    T get_expected_max(size_t i, size_t processCount)
    {
        if ((T)(i + processCount - 1) > T(i))
            return (T)(i + processCount - 1);
        return (T)i;
    } 
    virtual int Run(TypedTestParam <T> &param) = 0; 
    virtual int Check(TypedTestParam <T> &param) = 0;

};

class MainTest:public::testing::TestWithParam <TestParam> {
    template <typename T> 
    int Run(TestParam param);
public:
    int Test(TestParam param) {
        // printf("type = %d\n", param.dataType);
        switch (param.dataType) {
        case DT_CHAR:
            return Run <char>(param);
        case DT_INT:
            return Run <int>(param);
        // case DT_BFP16:
             // temporary
            // return TEST_SUCCESS;
        case DT_FLOAT:
            return Run <float>(param);
        case DT_DOUBLE:
            return Run <double>(param);
            // case DT_INT64:
            // temporary
            // return TEST_SUCCESS;
            // case DT_UINT64:
            // temporary
            // return TEST_SUCCESS;
        default:
            EXPECT_TRUE(false) << "Unknown data type: " << param.dataType;
            return TEST_FAILURE;
        }
    }
};
#endif /* BASE_HPP */
