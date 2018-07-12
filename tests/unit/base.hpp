#ifndef BASE_HPP
#define BASE_HPP

#include <sstream>

#include <sys/syscall.h> /* syscall */
#include <cmath>         /* pow */

#include "gtest/gtest.h"

#include "mlsl.hpp"

using namespace std;
using namespace MLSL;

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

#define PRINT(fmt, ...)
#define PRINT_BUFFER(buf, bufSize, prefix)

#endif /* ENABLE_DEBUG */


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

#define OUTPUT_NAME_ARG "--gtest_output="
//TODO: remove this:
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
                  ::testing::GTEST_FLAG(output) = patchedArg.c_str();                   \
              }                                                                         \
          }                                                                             \
      }                                                                                 \
  } while (0)

#define MAIN_FUNCTION()                         \
  int main(int argc, char **argv)               \
  {                                             \
      Environment::GetEnv().Init(&argc, &argv); \
      PATCH_OUTPUT_NAME_ARG(argc, argv);        \
      ::testing::InitGoogleTest(&argc, argv);   \
      int res = RUN_ALL_TESTS();                \
      Environment::GetEnv().Finalize();         \
      return res;                               \
  }

#define SESSION_OR_REGINFO_IS_NULL        ".*session or reg_info is null"
#define INVALID_OP_IDX                    ".*invalid operation idx"
#define INVALID_INPUT_ACT_IDX             ".*invalid input activation idx"
#define INVALID_OUTPUT_ACT_IDX            ".*invalid output activation idx"
#define ACT_SIZES_MUST_MATCH              ".*prev output activation size must match current input activation size"
#define DATA_TYPES_MUST_MATCH             ".*datatype must match"
#define PARTS_SHOULD_BE_POSITIVE          ".*numbers for data and model groups must be positive.*"
#define MB_SIZE_SHOULD_BE_POSITIVE        ".*minibatch size must be positive.*"
#define MB_CAN_BE_SET_ONLY_ONCE           ".*minibatch size can be set only once.*"
#define SET_GLOBAL_MB_SIZE                ".*set global mini-batch size.*"
#define DIST_CAN_BE_SET_ONLY_ONCE         ".*distribution can be set only once.*"
#define QUANT_PARAMS_CAN_BE_SET_ONLY_ONCE ".*quantization parameters can be set only once.*"
#define COUNT_AND_SIZE_SHOULD_BE_POSITIVE ".*count and size should be positive.*"
#define IS_NOT_SUPPORTED                  ".*is not supported yet.*"
#define NAME_IS_NULL                      ".*name is null.*"
#define UNSUPPORTED_YET                   ".*unsupported yet"
#define BATCH_SIZE_SHOULD_BE_SET_BEFORE   ".*global batch size should be set before operation creation"
#define COMMIT_SHOULD_BE_CALLED_ONCE      ".*commit should be called only once"
#define DISTRIBUTION_IS_NULL              "distribution is NULL.*"
#define SET_QUANT_PARAMS                  ".*set quantization parameters"

#define UNUSED_ATTR    __attribute__((unused))
#define DTYPE          float
#define DTYPE_SIZE     sizeof(DTYPE)
#define CACHELINE_SIZE 64
#define MLSL_DTYPE     DataType::DT_FLOAT

#define OP_TYPE_FIRST OpType::OT_CC
#define OP_TYPE_LAST  OpType::OT_EVAL

#define GLOBAL_MINIBATCH_SIZE 9240 /* 2 x 3 x 4 x 5 x 7 x 11 */

/* precreated networks parameters */
#define NETWORK_COUNT         1 /* same, diff, last */
#define OPERATION_COUNT       2
#define TRIPLET_COUNT         2

/* default values */
#define DEFAULT_OP_TYPE        OpType::OT_CC
#define DEFAULT_FM_COUNT       20
#define DEFAULT_FM_SIZE        256
#define DEFAULT_KERNEL_COUNT   (DEFAULT_FM_COUNT * DEFAULT_FM_COUNT)
#define DEFAULT_KERNEL_SIZE    9
#define DEFAULT_IN_ACT_COUNT   1
#define DEFAULT_OUT_ACT_COUNT  1
#define DEFAULT_PARAM_COUNT    1
#define DEFAULT_IS_DIST_UPDATE false
#define DEFAULT_DIST_COUNT     4 /* data, model, hybrid, last */
#define DEFAULT_OP_NAME        "default_op_name"

#define IS_OP_SUPPORTED(opType) (((opType == OpType::OT_SPLIT) || (opType == OpType::OT_REDUCE)) ? 0 : 1)

#define ITERATE_OVER_NETS_OPS_TRS(expression)                                  \
  do                                                                           \
  {                                                                            \
      for (NetworkType netType = (NetworkType)0; netType < NT_LAST; netType++) \
      {                                                                        \
          TestNetwork& net = nets[netType];                                    \
          Statistics* stats UNUSED_ATTR = net.mlslSession->GetStats();         \
          TestOperation (&ops)[OPERATION_COUNT] = net.ops;                     \
          for (size_t opIdx = 0; opIdx < OPERATION_COUNT; opIdx++)             \
          {                                                                    \
              TestOperation& op = ops[opIdx];                                  \
              TestDistribution& dist = op.dist;                                \
              TestTriplet* trs = op.trs;                                       \
              Operation* mlslOp UNUSED_ATTR = op.mlslOp;                       \
              Distribution* mlslDist UNUSED_ATTR = dist.mlslDist;              \
              for (size_t trIdx = 0; trIdx < TRIPLET_COUNT; trIdx++)           \
              {                                                                \
                  TestTriplet& tr = trs[trIdx];                                \
                  Activation* inAct UNUSED_ATTR = tr.inAct;                    \
                  Activation* outAct UNUSED_ATTR = tr.outAct;                  \
                  ParameterSet* param UNUSED_ATTR = tr.param;                  \
                  PRINT("ITERATE_OVER_NETS_OPS_TRS: "                          \
                        "netType: %d, opIdx: %zu, trIdx: %zu",                 \
                        netType, opIdx, trIdx);                                \
                  expression;                                                  \
              }                                                                \
          }                                                                    \
      }                                                                        \
  } while (0)

#define ITERATE_OVER_NETS_OPS(expression)                                      \
  do                                                                           \
  {                                                                            \
      for (NetworkType netType = (NetworkType)0; netType < NT_LAST; netType++) \
      {                                                                        \
          TestNetwork& net = nets[netType];                                    \
          Statistics* stats UNUSED_ATTR = net.mlslSession->GetStats();         \
          TestOperation (&ops)[OPERATION_COUNT] = net.ops;                     \
          for (size_t opIdx = 0; opIdx < OPERATION_COUNT; opIdx++)             \
          {                                                                    \
              TestOperation& op = ops[opIdx];                                  \
              TestDistribution& dist = op.dist;                                \
              TestTriplet* trs UNUSED_ATTR = op.trs;                           \
              Operation* mlslOp UNUSED_ATTR = op.mlslOp;                       \
              Distribution* mlslDist UNUSED_ATTR = dist.mlslDist;              \
              PRINT("ITERATE_OVER_NETS_OPS: "                                  \
                    "netType: %d, opIdx: %zu",                                 \
                    netType, opIdx);                                           \
              expression;                                                      \
          }                                                                    \
      }                                                                        \
  } while (0)

#define ITERATE_OVER_DEF_DISTS(expression)                                                 \
  do                                                                                       \
  {                                                                                        \
      Statistics* stats UNUSED_ATTR = defSession->GetStats();                              \
      for (DistType distType = (DistType)0; distType < DT_LAST; distType++)                \
      {                                                                                    \
            DistType otherDistType UNUSED_ATTR = ((DistType)(distType + 1) == DT_LAST)     \
                                                 ? (DistType)0 : (DistType)(distType + 1); \
            PRINT("ITERATE_OVER_DEF_DISTS: distType: %d", distType);                       \
            Distribution* dist UNUSED_ATTR = defDistributions[distType];                   \
            OperationParam opParam; opParam.distType = distType;                           \
            expression;                                                                    \
      }                                                                                    \
  } while (0)

#define ITERATE_OVER_DEF_DISTS_OPTYPES(expression)                                         \
  do                                                                                       \
  {                                                                                        \
      Statistics* stats UNUSED_ATTR = defSession->GetStats();                              \
      for (DistType distType = (DistType)0; distType < DT_LAST; distType++)                \
      {                                                                                    \
            DistType otherDistType UNUSED_ATTR = ((DistType)(distType + 1) == DT_LAST)     \
                                                 ? (DistType)0 : (DistType)(distType + 1); \
            Distribution* dist UNUSED_ATTR = defDistributions[distType];                   \
            OperationParam opParam; opParam.distType = distType;                           \
            for (int opType = (int)OP_TYPE_FIRST; opType <= (int)OP_TYPE_LAST; opType++)   \
            {                                                                              \
                opParam.opType = (OpType)opType;                                           \
                PRINT("ITERATE_OVER_DEF_DISTS_OPTYPES: distType: %d, opType: %d",          \
                      distType, opType);                                                   \
                expression;                                                                \
            }                                                                              \
      }                                                                                    \
  } while (0)

template<class TYPE>
string to_string(TYPE value)
{
    stringstream stream;
    stream << value;
    return stream.str();
}

typedef enum
{
    DT_DATA = 0,
    DT_MODEL,
    DT_HYBRID,
    DT_LAST
} DistType;

DistType& operator++(DistType& orig)
{
    if (orig != DT_LAST)
        orig = static_cast<DistType>(orig + 1);
    return orig;
}

DistType operator++(DistType& orig, int)
{
    DistType rVal = orig;
    ++orig;
    return rVal;
}

typedef enum
{
    NT_SAME_DISTS = 0,
    NT_LAST,
    NT_DIFF_DISTS, /* currently this case is not supported yet by MLSL */
} NetworkType;

NetworkType& operator++(NetworkType& orig)
{
    if (orig != NT_LAST)
        orig = static_cast<NetworkType>(orig + 1);
    return orig;
}

NetworkType operator++(NetworkType& orig, int)
{
    NetworkType rVal = orig;
    ++orig;
    return rVal;
}

class OperationParam
{
public:
    DistType distType;
    size_t inActCount;
    size_t outActCount;
    size_t paramCount;
    size_t fmCount;
    size_t fmSize;
    size_t kernelCount;
    size_t kernelSize;
    bool distUpdate;
    OpType opType;
    DataType dataType;
    const char* opName;

    OperationParam() : distType(DT_HYBRID),
                       inActCount(DEFAULT_IN_ACT_COUNT),
                       outActCount(DEFAULT_OUT_ACT_COUNT),
                       paramCount(DEFAULT_PARAM_COUNT),
                       fmCount(DEFAULT_FM_COUNT),
                       fmSize(DEFAULT_FM_SIZE),
                       kernelCount(DEFAULT_KERNEL_COUNT),
                       kernelSize(DEFAULT_KERNEL_SIZE),
                       distUpdate(DEFAULT_IS_DIST_UPDATE),
                       opType(DEFAULT_OP_TYPE),
                       dataType(MLSL_DTYPE),
                       opName(DEFAULT_OP_NAME)
    {}
};

/**
 *  Represents the triplet of input activation, parameter set and output activation
 */
class TestTriplet
{
public:
    size_t inActFmCount, inActFmSize;
    size_t outActFmCount, outActFmSize;
    size_t kernelCount, kernelSize;

    Activation* inAct;
    Activation* outAct;
    ParameterSet* param;

    void Print()
    {
        PRINT("inActFmCount: %zu, inActFmSize: %zu, "
               "outActFmCount: %zu, outActFmSize: %zu, "
               "kernelCount: %zu, kernelSize: %zu",
               inActFmCount, inActFmSize,
               outActFmCount, outActFmSize,
               kernelCount, kernelSize);
    }
};

class TestDistribution
{
public:
    Distribution* mlslDist;
    size_t processIdx, processCount;
    size_t dataProcessIdx, dataProcessCount;
    size_t modelProcessIdx, modelProcessCount;

    void Print()
    {
        PRINT("process_idx: %zu, process_count: %zu", processIdx, processCount);
        PRINT("data_process_idx: %zu, data_process_count: %zu", dataProcessIdx, dataProcessCount);
        PRINT("model_process_idx: %zu, model_process_count: %zu", modelProcessIdx, modelProcessCount);
    }
};

class TestOperation
{
public:
    TestTriplet trs[TRIPLET_COUNT];
    Operation* mlslOp;
    TestDistribution dist;

    void Print()
    {
        dist.Print();
        for (size_t trIdx = 0; trIdx < TRIPLET_COUNT; trIdx++)
        {
            PRINT("triplet_idx: %zu", trIdx);
            trs[trIdx].Print();
        }
    }
};

class TestNetwork
{
public:
    Session* mlslSession;
    TestOperation ops[OPERATION_COUNT];

    void Print()
    {
        for (size_t opIdx = 0; opIdx < OPERATION_COUNT; opIdx++)
        {
            PRINT("operation_idx: %zu", opIdx);
            ops[opIdx].Print();
        }
    }
};

class BaseTest : public ::testing::Test
{

protected:
    /* the set of precreated networks: TestNetwork->TestOperations->TestTriplets */
    TestNetwork nets[NETWORK_COUNT];

    /* default objects to reuse in test cases */
    Session* defSession;
    Distribution* defDistributions[DEFAULT_DIST_COUNT];
    int isMultiProcess;
    size_t globalProcessCount;

public:
    BaseTest()
    {
        globalProcessCount = Environment::GetEnv().GetProcessCount();
        ASSERT((globalProcessCount == 1) || ((globalProcessCount % 2 == 0) && (GLOBAL_MINIBATCH_SIZE % globalProcessCount == 0)),
              "globalProcessCount (%zu) should be equal to 1 or be divisible by 2 and be divisor of GLOBAL_MINIBATCH_SIZE (%d)",
              globalProcessCount, GLOBAL_MINIBATCH_SIZE);
        isMultiProcess = (globalProcessCount > 1) ? 1 : 0;
    }

    ~BaseTest()
    {}

    Distribution* CreateHybridDistribution()
    {
        Distribution* dist;
        if (isMultiProcess)
        {
            if (globalProcessCount == 2)
                dist = Environment::GetEnv().CreateDistribution(2, 1);
            else
                dist = Environment::GetEnv().CreateDistribution(globalProcessCount / 2, 2);
        }
        else
            dist = Environment::GetEnv().CreateDistribution(1, 1);
        return dist;
    }

    /* default objects */
    void CreateDefaultObjects()
    {
        defSession = Environment::GetEnv().CreateSession();
        defSession->SetGlobalMinibatchSize(GLOBAL_MINIBATCH_SIZE);
        defDistributions[DT_DATA] = Environment::GetEnv().CreateDistribution(globalProcessCount, 1);
        defDistributions[DT_MODEL] = Environment::GetEnv().CreateDistribution(1, globalProcessCount);
        defDistributions[DT_HYBRID] = CreateHybridDistribution();

    }

    OperationRegInfo* CreateDefaultOperationRegInfo(OperationParam& param)
    {
        OperationRegInfo* regInfo = defSession->CreateOperationRegInfo(param.opType);
        regInfo->SetName(param.opName);

        for (size_t idx = 0; idx < param.inActCount; idx++)
            regInfo->AddInput(param.fmCount, param.fmSize, param.dataType);

        for (size_t idx = 0; idx < param.outActCount; idx++)
            regInfo->AddOutput(param.fmCount, param.fmSize, param.dataType);

        for (size_t idx = 0; idx < param.paramCount; idx++)
            regInfo->AddParameterSet(param.kernelCount, param.kernelSize, param.dataType, param.distUpdate);

        return regInfo;
    }

    void DeleteDefaultOperationRegInfo(OperationRegInfo* regInfo)
    {
        defSession->DeleteOperationRegInfo(regInfo);
    }

    Operation* CreateDefaultOperation(OperationParam& param)
    {
        OperationRegInfo* regInfo = CreateDefaultOperationRegInfo(param);
        size_t opIdx = defSession->AddOperation(regInfo, defDistributions[param.distType]);
        defSession->DeleteOperationRegInfo(regInfo);
        return defSession->GetOperation(opIdx);
    }

    Operation* CreateOperationWithoutDistribution(OperationParam& param)
    {
        OperationRegInfo* regInfo = CreateDefaultOperationRegInfo(param);
        size_t opIdx = defSession->AddOperation(regInfo, NULL);
        defSession->DeleteOperationRegInfo(regInfo);
        return defSession->GetOperation(opIdx);
    }

    void DeleteDefaultObjets()
    {
        Environment::GetEnv().DeleteSession(defSession);
        for (DistType distType = (DistType)0; distType < DT_LAST; distType++)
            Environment::GetEnv().DeleteDistribution(defDistributions[distType]);
    }

    /* precreated networks */
    void CreateTestNetworks()
    {
        for (NetworkType netType = (NetworkType)0; netType < NT_LAST; netType++)
        {
            TestNetwork& net = nets[netType];
            if (netType == NT_SAME_DISTS)
            {
                PRINT("create network using the same distributions");

                Session* mlslSession = Environment::GetEnv().CreateSession();
                mlslSession->SetGlobalMinibatchSize(GLOBAL_MINIBATCH_SIZE);
                Distribution* mlslDist = CreateHybridDistribution();

                size_t processIdx = Environment::GetEnv().GetProcessIdx();
                size_t processCount = Environment::GetEnv().GetProcessCount();

                size_t dataProcessIdx = mlslDist->GetProcessIdx(GT_DATA);
                size_t dataProcessCount = mlslDist->GetProcessCount(GT_DATA);

                size_t modelProcessIdx = mlslDist->GetProcessIdx(GT_MODEL);
                size_t modelProcessCount = mlslDist->GetProcessCount(GT_MODEL);

                net.mlslSession = mlslSession;
                TestOperation (&ops)[OPERATION_COUNT] = net.ops;

                for (int opIdx = 0; opIdx < OPERATION_COUNT; opIdx++)
                {
                    TestOperation& op = ops[opIdx];
                    for (int trIdx = 0; trIdx < TRIPLET_COUNT; trIdx++)
                    {
                        TestTriplet& triplet = op.trs[trIdx];
                        triplet.inActFmCount =
                            (!opIdx) ? (modelProcessCount * (trIdx + 1)) : ops[opIdx - 1].trs[trIdx].outActFmCount;
                        triplet.inActFmSize =
                            (!opIdx) ? pow((16 + trIdx), 2): ops[opIdx - 1].trs[trIdx].outActFmSize;
                        triplet.outActFmCount = triplet.inActFmCount;
                        triplet.outActFmSize = triplet.inActFmSize;
                        triplet.kernelCount = triplet.inActFmCount * triplet.outActFmCount;
                        triplet.kernelSize = pow((opIdx + trIdx + 1), 2);

                        triplet.Print();
                    }

                    TestDistribution& dist = op.dist;
                    dist.mlslDist = mlslDist;
                    dist.processIdx = processIdx; dist.processCount = processCount;
                    dist.dataProcessIdx = dataProcessIdx; dist.dataProcessCount = dataProcessCount;
                    dist.modelProcessIdx = modelProcessIdx; dist.modelProcessCount = modelProcessCount;
                    CreateTestOperation(netType, opIdx);
                }
                mlslSession->Commit();
            }
            else if (netType == NT_DIFF_DISTS)
            {
                PRINT("create network using the different distributions");

                Session* mlslSession = Environment::GetEnv().CreateSession();
                mlslSession->SetGlobalMinibatchSize(GLOBAL_MINIBATCH_SIZE);

                net.mlslSession = mlslSession;
                TestOperation (&ops)[OPERATION_COUNT] = net.ops;

                for (int opIdx = 0; opIdx < OPERATION_COUNT; opIdx++)
                {
                    Distribution* mlslDist = CreateHybridDistribution();

                    size_t processIdx = Environment::GetEnv().GetProcessIdx();
                    size_t processCount = Environment::GetEnv().GetProcessCount();

                    size_t dataProcessIdx = mlslDist->GetProcessIdx(GT_DATA);
                    size_t dataProcessCount = mlslDist->GetProcessCount(GT_DATA);

                    size_t modelProcessIdx = mlslDist->GetProcessIdx(GT_MODEL);
                    size_t modelProcessCount = mlslDist->GetProcessCount(GT_MODEL);

                    TestOperation& op = ops[opIdx];
                    for (int trIdx = 0; trIdx < TRIPLET_COUNT; trIdx++)
                    {
                        TestTriplet& triplet = op.trs[trIdx];
                        triplet.inActFmCount =
                            (!opIdx) ? (modelProcessCount * (trIdx + 1)) : ops[opIdx - 1].trs[trIdx].outActFmCount;
                        triplet.inActFmSize =
                            (!opIdx) ? pow((16 + trIdx), 2) : ops[opIdx - 1].trs[trIdx].outActFmSize;
                        triplet.outActFmCount = triplet.inActFmCount;
                        triplet.outActFmSize = triplet.inActFmSize;
                        triplet.kernelCount = triplet.inActFmCount * triplet.outActFmCount;
                        triplet.kernelSize = pow((opIdx + trIdx + 1), 2);

                        triplet.Print();
                    }

                    TestDistribution& dist = op.dist;
                    dist.mlslDist = mlslDist;
                    dist.processIdx = processIdx; dist.processCount = processCount;
                    dist.dataProcessIdx = dataProcessIdx; dist.dataProcessCount = dataProcessCount;
                    dist.modelProcessIdx = modelProcessIdx; dist.modelProcessCount = modelProcessCount;

                    CreateTestOperation(netType, opIdx);
                }
                mlslSession->Commit();
            }
            else
                ASSERT(0, "unexpected network type");

            net.Print();
        }
    }

    void CreateTestOperation(NetworkType netType, size_t opIdx)
    {
        TestNetwork& net = nets[netType];
        TestOperation (&ops)[OPERATION_COUNT] = net.ops;
        TestOperation& op = ops[opIdx];
        TestTriplet (&triplets)[TRIPLET_COUNT] = ops[opIdx].trs;
        Session* mlslSession = net.mlslSession;
        Distribution* mlslDist = op.dist.mlslDist;
        OperationRegInfo* regInfo = mlslSession->CreateOperationRegInfo(DEFAULT_OP_TYPE);
        regInfo->SetName(to_string(opIdx).c_str());

        for (int trIdx = 0; trIdx < TRIPLET_COUNT; trIdx++)
        {
            TestTriplet& tr = triplets[trIdx];

            if (opIdx != 0)
                regInfo->AddInput(tr.inActFmCount,
                                  tr.inActFmSize,
                                  MLSL_DTYPE);

            regInfo->AddParameterSet(tr.kernelCount,
                                     tr.kernelSize,
                                     MLSL_DTYPE,
                                     (trIdx % 2) ? true : false /* dist_update */ );

            if (opIdx != (OPERATION_COUNT - 1))
                 regInfo->AddOutput(tr.outActFmCount,
                                   tr.outActFmSize,
                                   MLSL_DTYPE);
        }

        ASSERT(opIdx == mlslSession->AddOperation(regInfo, mlslDist), "different op indexes");
        op.mlslOp = mlslSession->GetOperation(opIdx);
        mlslSession->DeleteOperationRegInfo(regInfo);

        if (opIdx != 0)
        {
            for (int trIdx = 0; trIdx < TRIPLET_COUNT; trIdx++)
                op.mlslOp->SetPrev(ops[opIdx - 1].mlslOp, trIdx, trIdx);
        }

        for (int trIdx = 0; trIdx < TRIPLET_COUNT; trIdx++)
        {
            TestTriplet& tr = triplets[trIdx];

            if (opIdx != 0)
                tr.inAct = op.mlslOp->GetInput(trIdx);
            else
                tr.inAct = NULL;

            if (opIdx != (OPERATION_COUNT - 1))
                tr.outAct = op.mlslOp->GetOutput(trIdx);
            else
                tr.outAct = NULL;

            tr.param = op.mlslOp->GetParameterSet(trIdx);
        }
    }

    void SetDefaultQuantParams()
    {
        QuantParams* quantParams = new QuantParams();
        quantParams->lib_path = strdup("path_to_dl_lib");
        quantParams->quant_buffer_func_name = strdup("dl_comp_compress_buffer");
        quantParams->dequant_buffer_func_name = strdup("dl_comp_decompress_buffer");
        quantParams->reduce_sum_func_name = strdup("dl_comp_compressed_buffer_reduce_sum");
        quantParams->block_size = 268;
        quantParams->elem_in_block = 256;
        Environment::GetEnv().SetQuantizationParams(quantParams);
        free(quantParams->lib_path);
        free(quantParams->quant_buffer_func_name);
        free(quantParams->dequant_buffer_func_name);
        free(quantParams->reduce_sum_func_name);
        delete quantParams;
    }

    void DeleteTestNetworks()
    {
        for (NetworkType netType = (NetworkType)0; netType < NT_LAST; netType++)
        {
            TestNetwork& net = nets[netType];
            Environment::GetEnv().DeleteSession(net.mlslSession);

            if (netType == NT_SAME_DISTS)
            {
                Distribution* mlslDist = net.ops[0].dist.mlslDist;
                Environment::GetEnv().DeleteDistribution(mlslDist);
            }
            else if (netType == NT_DIFF_DISTS)
            {
                for (int opIdx = 0; opIdx < OPERATION_COUNT; opIdx++)
                {
                    Distribution* mlslDist = net.ops[opIdx].dist.mlslDist;
                    Environment::GetEnv().DeleteDistribution(mlslDist);
                }
            }
            else
                ASSERT(0, "unexpected network type");
        }
    }

    void SetUp()
    {
        CreateDefaultObjects();
        CreateTestNetworks();
    }

    void TearDown()
    {
        DeleteTestNetworks();
        DeleteDefaultObjets();
    }
};

#endif /* BASE_HPP */
