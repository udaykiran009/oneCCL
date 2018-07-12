#ifndef SOFTMAX_LOSS_LAYER_HPP
#define SOFTMAX_LOSS_LAYER_HPP

#include "base_layer.hpp"

class SoftmaxLossLayer : public BaseLayer
{
    size_t probsCount;
    DTYPE* probs;

public:
    SoftmaxLossLayer(LayerParam& param)
    {
        probsCount = LOCAL_MINIBATCH_SIZE * CLASS_COUNT;
        BaseLayer::CreateLayer(param);
        probs = (DTYPE*)MY_MALLOC(probsCount * DTYPE_SIZE);
        outputs[0] = (DTYPE*)MY_MALLOC(probsCount * DTYPE_SIZE);
        outputGrads[0] = (DTYPE*)MY_MALLOC(probsCount * DTYPE_SIZE); // not used
    }

    ~SoftmaxLossLayer()
    {
        MY_FREE(probs);
    }

    void Forward()
    {
        /* calculate probability vectors for each image */
        /*for (size_t imgIdx = 0; imgIdx < LOCAL_MINIBATCH_SIZE; ++imgIdx)
        {
            int selectedClass = rand() % CLASS_COUNT;
            for (size_t classIdx = 0; classIdx < CLASS_COUNT; classIdx++)
            {
                if (classIdx == selectedClass) probs[imgIdx * CLASS_COUNT + classIdx] = DTYPE(0.5);
                else probs[imgIdx * CLASS_COUNT + classIdx] = DTYPE(0.5) / (CLASS_COUNT - 1);
            }
        }*/

        /* there is no meaningful implementation, just stub */
        MY_ASSERT(outputs[0] && inputs[0], "buffers are null");
        memcpy(outputs[0], inputs[0], probsCount * DTYPE_SIZE);
    }

    void Backward()
    {
        /* there is no meaningful implementation, just stub */
        MY_ASSERT(inputGrads[0] && outputs[0], "buffers are null");
        memcpy(inputGrads[0], outputs[0], probsCount * DTYPE_SIZE);
        for (size_t imgIdx = 0; imgIdx < LOCAL_MINIBATCH_SIZE; ++imgIdx)
        {
            int selectedClass = rand() % CLASS_COUNT;
            for (size_t classIdx = 0; classIdx < CLASS_COUNT; classIdx++)
            {
                if (classIdx == selectedClass) inputGrads[0][imgIdx * CLASS_COUNT + classIdx] -= 1;
            }
        }
    }

    void Update()
    {
        /* no actions */
    }
};

#endif /* SOFTMAX_LOSS_LAYER_HPP */
