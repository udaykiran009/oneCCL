
/* ICCL library API usage example and correctness check test */

#include <math.h>   /* fabs */
#include <stdio.h>  /* printf */
#include <stdlib.h> /* exit */

#include "iccl.hpp"

using namespace ICCL;

/* Memory stuff */

#if defined(__INTEL_COMPILER) || defined(__ICC)
#define MY_MALLOC(size, align) _mm_malloc(size, align)
#define MY_FREE(ptr)           _mm_free(ptr)
#elif defined(__GNUC__)
#define MY_MALLOC(size, align) malloc(size)
#define MY_FREE(ptr)           free(ptr)
#else 
# error "this compiler is not supported" 
#endif


/* Logging stuff */

#define PRINT_BUF_LEN             4096
#define PRINT_BUF_FLUSH_THRESHOLD 3000
char printBuf[PRINT_BUF_LEN];
int printCount = 0;

#define MY_FLUSH()              \
  do                            \
  {                             \
      printBuf[printCount] = 0; \
      printf("%s", printBuf);   \
      printCount = 0;           \
      fflush(stdout);           \
  } while(0)

#define MY_PRINTF(...)                                                                  \
  do                                                                                    \
  {                                                                                     \
  if(processIdx == 0)                                                                         \
  {                                                                                     \
      int c = snprintf(printBuf + printCount, PRINT_BUF_LEN - printCount, __VA_ARGS__); \
      if (c > 0 && c < PRINT_BUF_LEN - printCount)                                      \
          printCount += c;                                                              \
      if (printCount > PRINT_BUF_FLUSH_THRESHOLD)                                       \
          MY_FLUSH();                                                                   \
  }                                                                                     \
  } while(0)

#define MY_ASSERT(cond,...)                                                   \
  do                                                                          \
  {                                                                           \
      if (!(cond))                                                            \
      {                                                                       \
          MY_FLUSH();                                                         \
          printf("%s:%d:assertion '%s' failed\n", __FILE__, __LINE__, #cond); \
          printf(__VA_ARGS__);                                                \
          Environment::GetEnv().Finalize();                                   \
          exit(1);                                                            \
      }                                                                       \
  } while(0)


/* ICCL Test stuff */

#define DTYPE                 float
#define DTYPE_SIZE            sizeof(DTYPE)
#define CACHELINE_SIZE        64
#define FAIL_COUNTER_MAX      5

#define GLOBAL_MINIBATCH_SIZE 16
#define LAYER_COUNT           2
#define EPOCH_COUNT           2
#define MINIBATCH_PER_EPOCH   1

class Layer;

Layer* layers[LAYER_COUNT];
Operation* operations[LAYER_COUNT];

size_t processIdx;
size_t processCount;

/* default parameters */
size_t groupCount = 1;
size_t replicaCount = 1;
bool useDistUpdate = true; // data parallelism's feature
bool useUserBuf = true;

int rank = 1;

enum LayerType
{
    CONV_MIMO = 0,
    CONV_FLAT = 1,
    FC        = 2
};

struct LayerParams
{
    size_t layerIdx;
    LayerType type;
    size_t ifm;
    size_t ofm;
    size_t ifmWidth;
    size_t ifmHeight;
    size_t ofmWidth;
    size_t ofmHeight;
    size_t kw;
    size_t kh;
};
LayerParams layerParams[LAYER_COUNT];

class Layer
{
    size_t layerIdx;
    Operation* op;
    DTYPE* inputActBuf, *outputActBuf;            /* input/output activations */
    DTYPE* inputActGradBuf, *outputActGradBuf;    /* gradients wrt input activations/ouput activations */
    DTYPE* paramBuf, *paramGradBuf, *paramIncBuf;  /* learnable parameters, gradient wrt parameters, parameters increment */
    bool isBackwardUnpackCalled;

public:
    Layer(size_t layerIdx, Operation* op, Layer* prevLayer) : layerIdx(layerIdx), op(op), isBackwardUnpackCalled(false)
    {
        MY_ASSERT(op->GetInput(0), "input activation is NULL");
        MY_ASSERT(op->GetParameterSet(0), "parameter is NULL");

        size_t inActSize, prevOutActSize;

        if (prevLayer == NULL)
            prevOutActSize = 0;
        else
        {
            MY_ASSERT(op->GetOutput(0), "output activation is NULL");
            prevOutActSize = prevLayer->op->GetOutput(0)->GetLocalFmCount()
                             * prevLayer->op->GetLocalMinibatchSize()
                             * prevLayer->op->GetOutput(0)->GetFmSize()
                             * DTYPE_SIZE;
        }

        inActSize = op->GetInput(0)->GetLocalFmCount() * op->GetLocalMinibatchSize() * op->GetInput(0)->GetFmSize() * DTYPE_SIZE;
        if (prevOutActSize > inActSize) inActSize = prevOutActSize;

        inputActBuf = (DTYPE*)MY_MALLOC(inActSize, CACHELINE_SIZE);
        inputActGradBuf = (DTYPE*)MY_MALLOC(inActSize, CACHELINE_SIZE);

        if (prevLayer != NULL)
        {
            prevLayer->outputActBuf = inputActBuf;
            prevLayer->outputActGradBuf = inputActGradBuf;
            op->SetPrev(prevLayer->op, 0, 0);
        }

        size_t paramBufSize = op->GetParameterSet(0)->GetLocalKernelCount()
                              * op->GetParameterSet(0)->GetKernelSize()
                              * DTYPE_SIZE;

        size_t paramBufIncSize = op->GetParameterSet(0)->GetOwnedKernelCount()
                                 * op->GetParameterSet(0)->GetKernelSize()
                                 * DTYPE_SIZE;

        if (useUserBuf)
        {
            paramBuf     = (DTYPE*)MY_MALLOC(paramBufSize, CACHELINE_SIZE);
            paramGradBuf = (DTYPE*)MY_MALLOC(paramBufSize, CACHELINE_SIZE);
            paramIncBuf  = (DTYPE*)MY_MALLOC(paramBufIncSize, CACHELINE_SIZE);
        }
        else
        {
            paramBuf     = (DTYPE*)Environment::GetEnv().Alloc(paramBufSize, CACHELINE_SIZE);
            paramGradBuf = (DTYPE*)Environment::GetEnv().Alloc(paramBufSize, CACHELINE_SIZE);
            paramIncBuf  = (DTYPE*)Environment::GetEnv().Alloc(paramBufIncSize, CACHELINE_SIZE);
        }

        MY_ASSERT(inputActBuf && inputActGradBuf && paramBuf && paramGradBuf && paramIncBuf,
                  "error while buffers allocating");

        for (size_t idx = 0; idx < paramBufSize / DTYPE_SIZE; idx++)
            paramBuf[idx] = idx;
    }

    ~Layer()
    {
        MY_FREE(inputActBuf);
        MY_FREE(inputActGradBuf);

        if (useUserBuf)
        {
            MY_FREE(paramBuf);
            MY_FREE(paramGradBuf);
            MY_FREE(paramIncBuf);
        }
        else
        {
            Environment::GetEnv().Free(paramBuf);
            Environment::GetEnv().Free(paramGradBuf);
            Environment::GetEnv().Free(paramIncBuf);
        }
    }

    void PackBuffer(Activation* act, DTYPE* commBuf, DTYPE* localBuf)
    {
        size_t localFmCount = act->GetLocalFmCount();
        for (size_t blockIdx = 0; blockIdx < act->GetPackBlockCount(); blockIdx++)
        {
            CommBlockInfo* blockInfo = act->GetPackBlock(blockIdx);
            size_t mbCount = blockInfo->GetMbCount();
            size_t mbOffset = blockInfo->GetMbOffset();
            size_t fmCount = blockInfo->GetFmCount();
            size_t fmOffset = blockInfo->GetFmOffset();
            size_t fmSize = blockInfo->GetFmSize();
            DTYPE* src = localBuf;
            DTYPE* dst = commBuf + blockInfo->GetBufOffset();
            for (size_t mbIdx = 0; mbIdx < mbCount; mbIdx++)
                for (size_t fmIdx = 0; fmIdx < fmCount; fmIdx++)
                    for (size_t spaceIdx = 0 ; spaceIdx < fmSize; spaceIdx++)
                        dst[mbIdx * fmCount * fmSize + fmIdx * fmSize + spaceIdx]
                            = src[(mbIdx + mbOffset) * localFmCount * fmSize + (fmIdx + fmOffset) * fmSize + spaceIdx];
        }
    }

    void UnpackBuffer(Activation* act, DTYPE* commBuf, DTYPE* localBuf)
    {
        size_t localFmCount = act->GetLocalFmCount();
        for (size_t blockIdx = 0; blockIdx < act->GetUnpackBlockCount(); blockIdx++)
        {
            CommBlockInfo* blockInfo = act->GetUnpackBlock(blockIdx);
            size_t mbCount = blockInfo->GetMbCount();
            size_t mbOffset = blockInfo->GetMbOffset();
            size_t fmCount = blockInfo->GetFmCount();
            size_t fmOffset = blockInfo->GetFmOffset();
            size_t fmSize = blockInfo->GetFmSize();
            DTYPE* src = commBuf + blockInfo->GetBufOffset();
            DTYPE* dst = localBuf;
            for (size_t mbIdx = 0; mbIdx < mbCount; mbIdx++)
                for (size_t fmIdx = 0; fmIdx < fmCount; fmIdx++)
                    for (size_t spaceIdx = 0 ; spaceIdx < fmSize; spaceIdx++)
                        dst[(mbIdx + mbOffset) * localFmCount * fmSize + (fmIdx + fmOffset) * fmSize + spaceIdx]
                            = src[mbIdx * fmCount * fmSize + fmIdx * fmSize + spaceIdx];
        }
    }

    void ForwardCompute(DTYPE* inputAct, DTYPE* param, DTYPE* outputAct)
    {
        if (layerIdx == 0)
        {
            /* Write to output activation */
            MY_ASSERT(op->GetOutput(0), "output activation is NULL");
            size_t outSize = op->GetOutput(0)->GetLocalFmCount() * op->GetLocalMinibatchSize() * op->GetOutput(0)->GetFmSize();
            for (size_t idx = 0; idx < outSize; idx++)
                outputAct[idx] = idx;
        }
        else if (layerIdx == 1)
        {
            /* Check for input activation */
            MY_ASSERT(op->GetInput(0), "input activation is NULL");
            size_t fmLocalCount = op->GetInput(0)->GetLocalFmCount();
            size_t mbLocalLen = op->GetLocalMinibatchSize();
            size_t fmSize = op->GetInput(0)->GetFmSize();
            size_t fmOffset =  op->GetInput(0)->GetGlobalFmOffset();
            size_t fmGroupSize = op->GetDistribution()->GetProcessCount(GT_MODEL);
            size_t failCounter = 0;
            //MY_PRINTF("[%zu] forward_%zu: input: idx %zu: expected %4.0f - received: %4.0f\n", rank, layerIdx, idx, expected, inputAct[idx]);
            int cntr = 0;
            for (size_t mbIdx = 0; mbIdx < mbLocalLen; mbIdx++)
            {
                for (size_t fmIdx = 0; fmIdx < fmLocalCount; fmIdx++)
                {
                    for (size_t spaceIdx = 0; spaceIdx < fmSize; spaceIdx++)
                    {
                        DTYPE expected = fmGroupSize * (mbIdx * fmLocalCount * fmSize * fmGroupSize + (fmOffset + fmIdx) * fmSize + spaceIdx);
                        size_t idx = mbIdx * fmLocalCount * fmSize + fmIdx * fmSize + spaceIdx;
                        if (fabs(inputAct[idx] - expected) > 1.e-4)
                        {
                            if (failCounter < FAIL_COUNTER_MAX)
                                MY_PRINTF("[%zu] forward_%zu: input: idx %zu: expected %4.0f - received: %4.0f\n",
                                          processIdx, layerIdx, idx, expected, inputAct[idx]);
                            failCounter++;
                        }
                        else
                        {
                            /*if (cntr++  == 5)
                                MY_PRINTF("[%zu] forward_%zu: input: idx %zu: expected %4.0f - received: %4.0f\n",
                                          processIdx, layerIdx, idx, expected, inputAct[idx]);
                                          */
                        }
                    }
                }
            }

            if (failCounter > 0)
            {
                MY_PRINTF("[%zu] forward_%zu: input activation test FAILED mismatch count = %zu\n", processIdx, layerIdx, failCounter);
                MY_ASSERT(0, "exit");
            }
            else
                MY_PRINTF("[%zu] forward_%zu: input activation test PASSED, my real rank is %d\n", processIdx, layerIdx, rank);
        }

        /* Now check ParameterSet */
        MY_ASSERT(op->GetParameterSet(0), "parameter is NULL");
        size_t paramSize = op->GetParameterSet(0)->GetLocalKernelCount() * op->GetParameterSet(0)->GetKernelSize();
        size_t failCounter = 0;
        for (size_t idx = 0; idx < paramSize; idx++)
        {
            if (fabs(param[idx] - idx) > 1.e-4)
            {
                if (failCounter < FAIL_COUNTER_MAX)
                    MY_PRINTF("[%zu] forward_%zu: parameter idx %zu: expected %4.0f - received: %4.0f\n",
                             processIdx,
                             layerIdx,
                             idx,
                             (DTYPE)idx,
                             param[idx]);
                failCounter++;
            }
        }

        if (failCounter > 0)
        {
            MY_PRINTF("[%zu] forward_%zu: parameter test FAILED mismatch count = %zu\n", processIdx, layerIdx, failCounter);
            MY_ASSERT(0, "exit");
        }
        else
            MY_PRINTF("[%zu] forward_%zu: parameter test PASSED\n", processIdx, layerIdx);
        MY_FLUSH();
    }

    void BackwardCompute1(DTYPE* outputActGrad, DTYPE* param, DTYPE* inputActGrad)
    {
        if (layerIdx == 0)
        {
            /* Check for inputs */
            MY_ASSERT(op->GetOutput(0), "output activation is NULL");
            size_t actSize = op->GetOutput(0)->GetLocalFmCount() * op->GetLocalMinibatchSize() * op->GetOutput(0)->GetFmSize();
            size_t failCounter = 0;
            for (size_t idx = 0; idx < actSize; idx++)
            {
                if (fabs(outputActGrad[idx] - idx) > 1.e-4)
                {
                    if (failCounter < FAIL_COUNTER_MAX)
                        MY_PRINTF("[%zu] backward_%zu: output activation gradient: idx %zu: expected %4.0f - received: %4.0f\n",
                                 processIdx,
                                 layerIdx,
                                 idx,
                                 (DTYPE)idx,
                                 outputActGrad[idx]);
                    failCounter++;
                }
            }
            if (failCounter > 0)
            {
                MY_PRINTF("[%zu] backward_%zu: output activation gradient test FAILED mismatch count = %zu\n", processIdx, layerIdx, failCounter);
                MY_ASSERT(0, "exit");
            }
            else
                MY_PRINTF("[%zu] backward_%zu: output activation gradient test PASSED\n", processIdx, layerIdx);
        }
        else if (layerIdx == 1)
        {
            /* Write to output */
            MY_ASSERT(op->GetInput(0), "input activation is NULL");
            size_t fmLocalCount = op->GetInput(0)->GetLocalFmCount();
            size_t mbLocalLen = op->GetLocalMinibatchSize();
            size_t fmSize = op->GetInput(0)->GetFmSize();
            size_t actOffset =  op->GetInput(0)->GetGlobalFmOffset();
            size_t groupSize = op->GetDistribution()->GetProcessCount(GT_MODEL);
            for (size_t mbIdx = 0; mbIdx < mbLocalLen; mbIdx++)
                for (size_t fmIdx = 0; fmIdx < fmLocalCount; fmIdx++)
                    for (size_t spaceIdx = 0; spaceIdx < fmSize; spaceIdx++)
                    {
                        size_t idx = mbIdx * fmLocalCount * fmSize + fmIdx * fmSize + spaceIdx;
                        inputActGrad[idx] = mbIdx * fmLocalCount * fmSize * groupSize + (actOffset + fmIdx) * fmSize + spaceIdx;
                    }
        }
        MY_FLUSH();
    }

    void BackwardCompute2(DTYPE* outputActGrad, DTYPE* inputAct, DTYPE* paramGrad)
    {
        MY_ASSERT(op->GetParameterSet(0), "parameter is NULL");
        size_t paramSize = op->GetParameterSet(0)->GetLocalKernelCount() * op->GetParameterSet(0)->GetKernelSize();
        for (size_t idx = 0; idx < paramSize; idx++)
            paramGrad[idx] = idx;
    }

    void UpdateCompute(DTYPE* paramGrad, DTYPE* paramInc, DTYPE* ownedParam, size_t ownedSize)
    {
        MY_ASSERT(op->GetParameterSet(0), "parameter is NULL");
        size_t mbGroupSize = op->GetDistribution()->GetProcessCount(GT_DATA);
        size_t ownedOffset = op->GetParameterSet(0)->GetOwnedKernelOffset() * op->GetParameterSet(0)->GetKernelSize();
        size_t failCounter = 0;
        int cntr = 0;
        for (size_t idx = 0; idx < ownedSize; idx++)
        {
            DTYPE expected = mbGroupSize * (ownedOffset + idx);
            if (fabs(paramGrad[idx] - expected) > 1.e-4)
                failCounter++;
            else
            {
                if (cntr++  == 5)
                    MY_PRINTF("[%zu] UpdateCompute_%zu: input: idx %zu: expected %4.0f - received: %4.0f\n",
                                          rank, layerIdx, idx, expected, paramGrad[idx]);
            }
            ownedParam[idx] = ownedOffset + idx;
        }
        if (failCounter > 0)
        {
            MY_PRINTF("[%zu] update_%zu: parameter gradient test FAILED mismatch count = %zu\n", processIdx, layerIdx, failCounter);
            MY_ASSERT(0, "exit");
        }
        else
            MY_PRINTF("[%zu] update_%zu: parameter gradient test PASSED\n", processIdx, layerIdx);
        MY_FLUSH();
    }

    /* Recv parameter increments (in case of distributed update) and input activations, send output activations */
    void Forward()
    {
        MY_ASSERT(op->GetInput(0), "input activation is NULL");
        MY_ASSERT(op->GetOutput(0), "otput activation is NULL");

        Activation* act = op->GetInput(0);
        DTYPE* commBuf = (DTYPE*)act->WaitComm();
        UnpackBuffer(act, commBuf, inputActBuf);
        if (op->HasParameterSets())
        {
            MY_ASSERT(op->GetParameterSet(0), "parameter is NULL");
            op->GetParameterSet(0)->WaitIncrementComm();
        }

        ForwardCompute(inputActBuf, paramBuf, outputActBuf);

        act = op->GetOutput(0);
        DTYPE* outputActCommBuf = (DTYPE*)act->GetCommBuf();
        PackBuffer(act, outputActCommBuf, outputActBuf);
        act->StartComm(outputActCommBuf);
        isBackwardUnpackCalled = false;
    }

    /* Calculate gradient wrt input activation and send it */
    void Backward1()
    {
        MY_ASSERT(op->GetInput(0), "input activation is NULL");

        if (!isBackwardUnpackCalled)
        {
            MY_ASSERT(op->GetOutput(0), "output activation is NULL");
            Activation* act = op->GetOutput(0);
            DTYPE* commBuf = (DTYPE*)act->WaitComm();
            UnpackBuffer(act, commBuf, outputActGradBuf);
            isBackwardUnpackCalled = true;
        }

        BackwardCompute1(outputActGradBuf, paramBuf, inputActGradBuf);

        Activation* act = op->GetInput(0);
        DTYPE* inputActCommBuf = (DTYPE*)act->GetCommBuf();
        PackBuffer(act, inputActCommBuf, inputActGradBuf);
        act->StartComm(inputActCommBuf);
    }

    /* Calculate gradient wrt parameters and send it */
    void Backward2()
    {
        if (!isBackwardUnpackCalled)
        {
            MY_ASSERT(op->GetOutput(0), "output activation is NULL");
            Activation* act = op->GetOutput(0);
            DTYPE* commBuf = (DTYPE*)act->WaitComm();
            UnpackBuffer(act, commBuf, outputActGradBuf);
            isBackwardUnpackCalled = true;
        }

        BackwardCompute2(outputActGradBuf, inputActBuf, paramGradBuf);
        if (op->HasParameterSets())
        {
            MY_ASSERT(op->GetParameterSet(0), "parameter is NULL");
            op->GetParameterSet(0)->StartGradientComm(paramGradBuf);
        }
    }

    /* Recv gradient wrt parameters and update parameters/send parameter increments (in case of distributed update) */
    void Update()
    {
        if (op->HasParameterSets())
        {
            MY_ASSERT(op->GetParameterSet(0), "parameter is NULL");
            DTYPE* commBuf = (DTYPE*)op->GetParameterSet(0)->WaitGradientComm();
            DTYPE* ownedParamBuf = paramBuf + op->GetParameterSet(0)->GetOwnedKernelOffset() * op->GetParameterSet(0)->GetKernelSize();
            UpdateCompute(commBuf == NULL ? paramGradBuf : commBuf,
                          paramIncBuf,
                          ownedParamBuf,
                          op->GetParameterSet(0)->GetOwnedKernelCount() * op->GetParameterSet(0)->GetKernelSize());
            op->GetParameterSet(0)->StartIncrementComm(paramBuf);
        }
    }
};

/* Layer initialization */
Layer* CreateLayer(Session* session, LayerType type, LayerParams* lParams, Distribution* distribution, Layer* prevLayer)
{
    MY_ASSERT((type == CONV_MIMO || type == CONV_FLAT || type == FC), "incorrect op type");

    size_t layerIdx = lParams->layerIdx;

    OperationRegInfo* regInfo = session->CreateOperationRegInfo(OT_CC);
    regInfo->SetName("MyLayerName");
    regInfo->AddInput(lParams->ifm, lParams->ifmWidth * lParams->ifmHeight, (DTYPE_SIZE == 4) ? DT_FLOAT : DT_DOUBLE);
    regInfo->AddOutput(lParams->ofm, lParams->ofmWidth * lParams->ofmHeight, (DTYPE_SIZE == 4) ? DT_FLOAT : DT_DOUBLE);
    regInfo->AddParameterSet(lParams->ifm * lParams->ofm, lParams->kw * lParams->kh, (DTYPE_SIZE == 4) ? DT_FLOAT : DT_DOUBLE, useDistUpdate);

    size_t opIdx = session->AddOperation(regInfo, distribution);
    session->DeleteOperationRegInfo(regInfo);

    Operation* op = session->GetOperation(opIdx);
    operations[layerIdx] = op;
    Layer* layer = new Layer(layerIdx, op, prevLayer);

    return layer;
}

int main(int argc, char** argv)
{
    if (argc != 5)
    {
        printf("specify parameters: iccl_test [GROUP_COUNT] [DIST_UPDATE] [USER_BUF] [REPLICA_COUNT]\n");
        exit(0);
    }

    int runtime_version = Environment::GetEnv().GetVersion();
    printf("built with ICCL API version: %d.%d, used ICCL API version: %d.%d\n",
           ICCL_MAJOR_VERSION, ICCL_MINOR_VERSION, ICCL_MAJOR(runtime_version), ICCL_MINOR(runtime_version));

    if (ICCL_MAJOR_VERSION != ICCL_MAJOR(runtime_version))
    {
        printf("incompatible ICCL API version: %d.%d, exit\n",
               ICCL_MAJOR(runtime_version), ICCL_MINOR(runtime_version));
        return 0;
    }

    Environment::GetEnv().Init(&argc, &argv);

    rank = Environment::GetEnv().GetProcessIdx();
    int comm_world_size = Environment::GetEnv().GetProcessCount();
    if(comm_world_size < 4)
    {
        printf("Please run not less than 4 ranks\n");
        exit(0);
    }
    if(rank>1)
        Environment::GetEnv().Configure("color=3");
    else
        Environment::GetEnv().Configure("color=2");

    if(rank > 1)
    {

        Session* session = Environment::GetEnv().CreateSession();
        session->SetGlobalMinibatchSize(GLOBAL_MINIBATCH_SIZE);
        processCount = Environment::GetEnv().GetProcessCount();

        if (argc > 1) groupCount    = atoi(argv[1]);
        if (argc > 2) useDistUpdate = (atoi(argv[2]) != 0);
        if (argc > 3) useUserBuf    = (atoi(argv[3]) != 0);
        if (argc > 4) replicaCount    = (atoi(argv[4]) != 0) ? atoi(argv[4]) : 1;

        if (groupCount < 1) groupCount = 1;
        if (groupCount > processCount) groupCount = processCount;

        if(processCount % replicaCount != 0)
        {
            printf("[REPLICA_COUNT] needs to be a denominator of (number of ranks - 2)\n");
            exit(0);
        }

        if (rank == 0)
            printf("\nprocess_count = %zu, distribution = %zu x %zu x %zu(data_parts x model_parts x replica_count), dist_update %d, user_buf %d\n\n",
                   processCount, processCount/groupCount/replicaCount, groupCount, replicaCount, useDistUpdate, useUserBuf);

        /* Correctness test assumes both the layers use same distribution */
        Distribution* distribution = Environment::GetEnv().CreateDistribution(processCount/groupCount/replicaCount, groupCount);
        processIdx = distribution->GetProcessIdx(GT_DATA);

        /* Init all the layers */
        size_t layerIdx;
        for (layerIdx = 0; layerIdx < LAYER_COUNT; layerIdx++)
        {
            /* Set layerParams for each layer */
            if (layerIdx == 0)
            {
                layerParams[layerIdx].layerIdx = layerIdx;
                layerParams[layerIdx].type = CONV_MIMO;
                layerParams[layerIdx].ifm  = 128;
                layerParams[layerIdx].ofm  = 256;
                layerParams[layerIdx].ifmWidth = 12;
                layerParams[layerIdx].ifmHeight = 12;
                layerParams[layerIdx].ofmWidth = 12;
                layerParams[layerIdx].ofmHeight = 12;
                layerParams[layerIdx].kw = 3;
                layerParams[layerIdx].kh = 3;
            }
            else if (layerIdx == 1)
            {
                layerParams[layerIdx].layerIdx = layerIdx;
                layerParams[layerIdx].type = CONV_MIMO;
                layerParams[layerIdx].ifm  = 256;
                layerParams[layerIdx].ofm  = 256;
                layerParams[layerIdx].ifmWidth = 12;
                layerParams[layerIdx].ifmHeight = 12;
                layerParams[layerIdx].ofmWidth = 12;
                layerParams[layerIdx].ofmHeight = 12;
                layerParams[layerIdx].kw = 3;
                layerParams[layerIdx].kh = 3;
            }

            layers[layerIdx] = CreateLayer(session,
                                           layerParams[layerIdx].type,
                                           &layerParams[layerIdx],
                                           distribution,
                                           (layerIdx == 0 ? NULL : layers[layerIdx - 1]));
        }

        session->Commit();

        for (size_t epochIdx = 0; epochIdx < EPOCH_COUNT; epochIdx++)
        {
            for (size_t mbIdx = 0; mbIdx < MINIBATCH_PER_EPOCH; mbIdx++)
            {
                for (layerIdx = 0; layerIdx < LAYER_COUNT; layerIdx++)
                    layers[layerIdx]->Forward();

                for (layerIdx = 0; layerIdx < LAYER_COUNT; layerIdx++)
                {
                    /* Split backward phase on 2 steps to achieve comp/comm overlapping */
                    layers[LAYER_COUNT - layerIdx - 1]->Backward1();
                    layers[LAYER_COUNT - layerIdx - 1]->Backward2();
                }

                for (layerIdx = 0; layerIdx < LAYER_COUNT; layerIdx++)
                    layers[layerIdx]->Update();
            }

            /* Finish ParameterSet comms before ending epoch */
            for (layerIdx = 0; layerIdx < LAYER_COUNT; layerIdx++)
            {
                Operation* op = operations[layerIdx];
                if (op->HasParameterSets())
                {
                    MY_ASSERT(op->GetParameterSet(0), "parameter is NULL");
                    op->GetParameterSet(0)->WaitIncrementComm();
                }
            }
        }

        for (layerIdx = 0; layerIdx < LAYER_COUNT; layerIdx++)
            delete layers[layerIdx];

        Environment::GetEnv().DeleteSession(session);
        Environment::GetEnv().DeleteDistribution(distribution);
    }
    Environment::GetEnv().Finalize();

    printf("[%zu] exited normally\n", processIdx);

    return 0;
}
