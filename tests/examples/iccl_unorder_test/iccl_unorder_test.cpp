
/* ICCL library API unordered communication test */

#include <stdio.h>  /* printf */
#include <stdlib.h> /* exit */

#include "iccl.hpp"

using namespace ICCL;

#define MY_ASSERT(cond,...)                                                   \
  do                                                                          \
  {                                                                           \
      if (!(cond))                                                            \
      {                                                                       \
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
#define LAYER_COUNT           4
#define EPOCH_COUNT           2
#define MINIBATCH_PER_EPOCH   2

class Layer;

Layer* layers[LAYER_COUNT];
Operation* operations[LAYER_COUNT];
Distribution* distributions[LAYER_COUNT];

size_t processIdx;
size_t processCount;

struct LayerParams
{
    size_t layerIdx;
    size_t ifm;
    size_t ofm;
    size_t kw;
    size_t kh;
};
LayerParams layerParams[LAYER_COUNT];

class Layer
{
    size_t layerIdx;
    Operation* op;
    DTYPE* paramBuf, *paramGradBuf;  /* learnable parameters, gradient wrt parameters */

public:
    Layer(size_t layerIdx, Operation* op, Layer* prevLayer) : layerIdx(layerIdx), op(op)
    {
        size_t paramBufSize = op->GetParameterSet(0)->GetLocalKernelCount()
                              * op->GetParameterSet(0)->GetKernelSize()
                              * DTYPE_SIZE;

        paramBuf     = (DTYPE*)Environment::GetEnv().Alloc(paramBufSize, CACHELINE_SIZE);
        paramGradBuf = (DTYPE*)Environment::GetEnv().Alloc(paramBufSize, CACHELINE_SIZE);

        MY_ASSERT(paramBuf && paramGradBuf, "error while buffers allocating");

        for (size_t idx = 0; idx < paramBufSize / DTYPE_SIZE; idx++)
            paramBuf[idx] = layerIdx;
    }

    ~Layer()
    {
        Environment::GetEnv().Free(paramBuf);
        Environment::GetEnv().Free(paramGradBuf);
    }

    void Forward()
    {}

    /* Calculate gradient wrt parameters and send it */
    void Backward()
    {
        MY_ASSERT(op->GetParameterSet(0), "parameter is NULL");
        size_t paramSize = op->GetParameterSet(0)->GetLocalKernelCount() * op->GetParameterSet(0)->GetKernelSize();
        for (size_t idx = 0; idx < paramSize; idx++)
            paramGradBuf[idx] = layerIdx;
        op->GetParameterSet(0)->StartGradientComm(paramGradBuf);
    }

    /* Recv gradient wrt parameters and update parameters */
    void Update()
    {
        MY_ASSERT(op->GetParameterSet(0), "parameter is NULL");
        size_t paramSize = op->GetParameterSet(0)->GetLocalKernelCount() * op->GetParameterSet(0)->GetKernelSize();
        DTYPE* commBuf = (DTYPE*)op->GetParameterSet(0)->WaitGradientComm();

        size_t fail_counter = 0;
        for (size_t idx = 0; idx < paramSize; idx++)
            if (commBuf[idx] != DTYPE(processCount * layerIdx))
            {
                fail_counter++;
                printf("ERROR: idx = %zu, commBuf = %f, expected = %f\n", idx, commBuf[idx], DTYPE(processCount * layerIdx));

                if (fail_counter > 1)
                    return;
            }
    }
};

/* Layer initialization */
Layer* CreateLayer(Session* session, LayerParams* lParams, Distribution* distribution, Layer* prevLayer)
{
    size_t layerIdx = lParams->layerIdx;

    OperationRegInfo* regInfo = session->CreateOperationRegInfo(OT_CC);
    regInfo->SetName("MyLayerName");
    regInfo->AddParameterSet(lParams->ifm * lParams->ofm, lParams->kw * lParams->kh, (DTYPE_SIZE == 4) ? DT_FLOAT : DT_DOUBLE, false);

    size_t opIdx = session->AddOperation(regInfo, distribution);
    session->DeleteOperationRegInfo(regInfo);

    Operation* op = session->GetOperation(opIdx);
    operations[layerIdx] = op;
    Layer* layer = new Layer(layerIdx, op, prevLayer);

    return layer;
}

int main(int argc, char** argv)
{
    if (argc != 1)
    {
        printf("specify parameters: iccl_unorder_test\n");
        exit(0);
    }

    Environment::GetEnv().Init(&argc, &argv);
    Session* session = Environment::GetEnv().CreateSession();
    session->SetGlobalMinibatchSize(GLOBAL_MINIBATCH_SIZE);
    processCount = Environment::GetEnv().GetProcessCount();
    processIdx = Environment::GetEnv().GetProcessIdx();

    printf("process_idx %zu\n", processIdx);

    /* Init all the layers */
    size_t layerIdx;
    for (layerIdx = 0; layerIdx < LAYER_COUNT; layerIdx++)
    {
        layerParams[layerIdx].layerIdx = layerIdx;
        layerParams[layerIdx].ifm  = 3;
        layerParams[layerIdx].ofm  = 3;

        /* use different sizes for parameters to pass over different transport protocols */
        if (layerIdx % 2)
        {
            layerParams[layerIdx].kw = layerIdx + 1;
            layerParams[layerIdx].kh = 2;
        }
        else
        {
            layerParams[layerIdx].kw = layerIdx * 1000000 + 1;
            layerParams[layerIdx].kh = 2;
        }

        distributions[layerIdx] = Environment::GetEnv().CreateDistribution(processCount, 1);
        layers[layerIdx] = CreateLayer(session,
                                       &layerParams[layerIdx],
                                       distributions[layerIdx],
                                       (layerIdx == 0 ? NULL : layers[layerIdx - 1]));
    }

    session->Commit();

    for (size_t epochIdx = 0; epochIdx < EPOCH_COUNT; epochIdx++)
    {
        for (size_t mbIdx = 0; mbIdx < MINIBATCH_PER_EPOCH; mbIdx++)
        {
            for (layerIdx = 0; layerIdx < LAYER_COUNT; layerIdx++)
                layers[layerIdx]->Forward();

            /* start gradient communication in different orders */
            if (processIdx % 2)
            {
                for (layerIdx = 0; layerIdx < LAYER_COUNT; layerIdx++)
                {
                    printf("process_idx %zu, do Backward for layer %zu\n", processIdx, layerIdx); fflush(stdout);
                    layers[LAYER_COUNT - layerIdx - 1]->Backward();
                }
            }
            else
            {
                for (layerIdx = 0; layerIdx < LAYER_COUNT; layerIdx++)
                {
                    printf("process_idx %zu, do Backward for layer %zu\n", processIdx, layerIdx); fflush(stdout);
                    layers[layerIdx]->Backward();
                }
            }

            for (layerIdx = 0; layerIdx < LAYER_COUNT; layerIdx++)
            {
                printf("process_idx %zu, before Update for layer %zu\n", processIdx, layerIdx); fflush(stdout);
                layers[layerIdx]->Update();
                printf("process_idx %zu, after Update for layer %zu\n", processIdx, layerIdx); fflush(stdout);
            }
        }
    }

    for (layerIdx = 0; layerIdx < LAYER_COUNT; layerIdx++)
    {
        delete layers[layerIdx];
        Environment::GetEnv().DeleteDistribution(distributions[layerIdx]);
    }

    Environment::GetEnv().DeleteSession(session);
    Environment::GetEnv().Finalize();

    printf("[%zu] exited normally\n", processIdx);

    return 0;
}
