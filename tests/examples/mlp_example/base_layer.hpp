#ifndef BASE_LAYER_HPP
#define BASE_LAYER_HPP

#include "common.hpp"

#include <vector>

using namespace std;

typedef struct
{
    size_t idx;
    size_t inputCount;
    size_t outputCount;
    size_t paramCount;
} LayerParam;

class BaseLayer
{

protected:
    vector<DTYPE*> inputs;
    vector<DTYPE*> outputs;
    vector<DTYPE*> inputGrads;
    vector<DTYPE*> outputGrads;
    vector<DTYPE*> params;
    vector<DTYPE*> paramGrads;

public:
    void CreateLayer(LayerParam& param)
    {
        inputs.resize(param.inputCount, (DTYPE*)NULL);
        outputs.resize(param.outputCount, (DTYPE*)NULL);

        inputGrads.resize(param.inputCount, (DTYPE*)NULL);
        outputGrads.resize(param.outputCount, (DTYPE*)NULL);

        params.resize(param.paramCount, (DTYPE*)NULL);
        paramGrads.resize(param.paramCount, (DTYPE*)NULL);
    }

    BaseLayer() {}

    virtual ~BaseLayer()
    {
        for (size_t idx = 0; idx < outputs.size(); idx++)
            MY_FREE(outputs[idx]);

        for (size_t idx = 0; idx < outputGrads.size(); idx++)
            MY_FREE(outputGrads[idx]);

        for (size_t idx = 0; idx < params.size(); idx++)
            MY_FREE(params[idx]);

        for (size_t idx = 0; idx < paramGrads.size(); idx++)
            MY_FREE(paramGrads[idx]);

        inputs.clear();
        outputs.clear();
        inputGrads.clear();
        outputGrads.clear();
        params.clear();
        paramGrads.clear();
    }

    void SetPrev(BaseLayer* prevLayer, size_t idx)
    {
        inputs[idx] = prevLayer->outputs[idx];
        inputGrads[idx] = prevLayer->outputGrads[idx];
    }

    virtual void Forward() = 0;
    virtual void Backward() = 0;
    virtual void Update() = 0;
};

#endif /* BASE_LAYER_HPP */
