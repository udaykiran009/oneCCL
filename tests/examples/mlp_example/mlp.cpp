#include "common.hpp"
#include "data_layer.hpp"
#include "inner_product_layer.hpp"
#include "softmax_loss_layer.hpp"

#if USE_ICCL
#include "iccl_wrapper.hpp"
#endif

BaseLayer* layers[LAYER_COUNT];

BaseLayer* CreateLayer(size_t layerIdx)
{
    BaseLayer* layer = NULL;
    LayerParam param;
    param.idx = layerIdx;

    if (layerIdx == 0)
    {        
        param.inputCount = 0;
        param.outputCount = 2; /* data + labels */
        param.paramCount = 0;
        layer = new DataLayer(param);
    }
    else if (layerIdx > 0 && layerIdx < LAYER_COUNT - 1)
    {
        param.inputCount = 1;
        param.outputCount = 1;
        param.paramCount = 1;
        layer = new InnerProductLayer(param);
        layer->SetPrev(layers[layerIdx - 1], 0);
    }
    else if (layerIdx == LAYER_COUNT - 1)
    {
        param.inputCount = 2; /* data + labels */
        param.outputCount = 1;
        param.paramCount = 0;
        layer = new SoftmaxLossLayer(param);
        layer->SetPrev(layers[layerIdx - 1], 0);
        layer->SetPrev(layers[0], 1);
    }

    return layer;
}

int main(int argc, char** argv)
{
#if USE_ICCL
    printf("use ICCL\n");
    InitICCL(argc, argv);
#else
    printf("don't use ICCL\n");
#endif

    size_t layerIdx;
    for (layerIdx = 0; layerIdx < LAYER_COUNT; layerIdx++)
        layers[layerIdx] = CreateLayer(layerIdx);

#if USE_ICCL
    session->Commit();
#endif

    for (size_t epochIdx = 0; epochIdx < EPOCH_COUNT; epochIdx++)
    {
        for (size_t mbIdx = 0; mbIdx < MINIBATCH_PER_EPOCH; mbIdx++)
        {
            for (layerIdx = 0; layerIdx < LAYER_COUNT; layerIdx++)
                layers[layerIdx]->Forward();

            for (layerIdx = 0; layerIdx < LAYER_COUNT; layerIdx++)
                layers[LAYER_COUNT - layerIdx - 1]->Backward();

            for (layerIdx = 0; layerIdx < LAYER_COUNT; layerIdx++)
                layers[layerIdx]->Update();

            printf("epochIdx: %zu, mbIdx: %zu - completed\n", epochIdx, mbIdx);
        }
    }

    for (layerIdx = 0; layerIdx < LAYER_COUNT; layerIdx++)
        delete layers[layerIdx];

#if USE_ICCL
    FinalizeICCL();
#endif

    printf("exited normally\n");

    return 0;
}
