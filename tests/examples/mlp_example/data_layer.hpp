#ifndef DATA_LAYER_HPP
#define DATA_LAYER_HPP

#include "base_layer.hpp"

class DataLayer : public BaseLayer
{

public:
    DataLayer(LayerParam& param)
    {
        BaseLayer::CreateLayer(param);
        outputs[0] = (DTYPE*)(MY_MALLOC(LOCAL_MINIBATCH_SIZE * IMAGE_SIZE * DTYPE_SIZE));
        outputs[1] = (DTYPE*)MY_MALLOC(LOCAL_MINIBATCH_SIZE * DTYPE_SIZE);
        srand(123);
    }

    ~DataLayer() {}

    void Forward()
    {
        MY_ASSERT(outputs[0] && outputs[1], "output buffers are null");
        for (size_t imgIdx = 0; imgIdx < LOCAL_MINIBATCH_SIZE; imgIdx++)
        {
            for (size_t pixelIdx = 0; pixelIdx < IMAGE_SIZE; pixelIdx++)
            {
                outputs[0][imgIdx * IMAGE_SIZE + pixelIdx] = (DTYPE)(rand() % 255);
            }
            outputs[1][imgIdx] = (DTYPE)(rand() % 10);
        }
    }

    void Backward()
    {
        /* no actions */
    }

    void Update()
    {
        /* no actions */
    }
};

#endif /* DATA_LAYER_HPP */
