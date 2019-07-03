#ifndef INNER_PRODUCT_LAYER_HPP
#define INNER_PRODUCT_LAYER_HPP

#include <cstring> /* memcpy */

#include "mkl.h"

#include "base_layer.hpp"

#if USE_ICCL
#include "iccl_wrapper.hpp"
#endif

class InnerProductLayer : public BaseLayer
{

private:
    size_t rowCount, columnCount, elemCount;
    size_t paramSize;
    DTYPE* history; /* for SGD */

#if USE_ICCL
    Operation* op;
#endif

public:
    InnerProductLayer(LayerParam& param)
    {
        BaseLayer::CreateLayer(param);

        size_t outputSize = LOCAL_MINIBATCH_SIZE * CLASS_COUNT * DTYPE_SIZE;
        outputs[0] = (DTYPE*)MY_MALLOC(outputSize);
        outputGrads[0] = (DTYPE*)MY_MALLOC(outputSize);

        rowCount = (param.idx == 1) ? IMAGE_SIZE : CLASS_COUNT;
        columnCount = CLASS_COUNT;
        elemCount = rowCount * columnCount;
        paramSize = elemCount * DTYPE_SIZE;
        params[0] = (DTYPE*)MY_MALLOC(paramSize);
        paramGrads[0] = (DTYPE*)MY_MALLOC(paramSize);
        history = (DTYPE*)MY_MALLOC(paramSize);

        for (size_t idx = 0; idx < elemCount; idx++)
            params[0][idx] = (DTYPE)((rand() % 100) / 10000.0);

#if USE_ICCL
        OperationRegInfo* regInfo = session->CreateOperationRegInfo(OT_CC);
        regInfo->AddParameterSet(elemCount, 1, ICCL_DTYPE);
        size_t opIdx = session->AddOperation(regInfo, distribution);
        op = session->GetOperation(opIdx);
        session->DeleteOperationRegInfo(regInfo);

        Bcast(params[0], elemCount);
#endif
    }

    ~InnerProductLayer() { MY_FREE(history); }

    void Forward()
    {
        MY_ASSERT(inputs[0] && outputs[0] && params[0], "buffers are null");

        /* C := alpha * op(A) * op(B) + beta * C */
        cblas_sgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
                    LOCAL_MINIBATCH_SIZE,                  /* number of rows in A and C */
                    CLASS_COUNT,                           /* number of columns in B and C */
                    rowCount,                              /* number of columns in A and number of rows in B */
                    (DTYPE)1.,                             /* alfa */
                    inputs[0],                             /* A */
                    rowCount,                              /* leading dimension of A */
                    params[0],                             /* B */
                    CLASS_COUNT,                           /* leading dimension of B */
                    (DTYPE)0.,                             /* beta */
                    outputs[0],                            /* C */
                    CLASS_COUNT);                          /* leading dimension of C */
    }

    void Backward()
    {
        MY_ASSERT(outputGrads[0] && inputs[0] && paramGrads[0], "buffers are null");

        cblas_sgemm(CblasRowMajor, CblasTrans, CblasNoTrans,
                    CLASS_COUNT,                            /* number of rows in A and C */
                    rowCount,                               /* number of columns in B and C */
                    LOCAL_MINIBATCH_SIZE,                   /* number of columns in A and number of rows in B */
                    (DTYPE)1.,                              /* alfa */
                    outputGrads[0],                         /* A */
                    CLASS_COUNT,                            /* leading dimension of A */
                    inputs[0],                              /* B */
                    rowCount,                               /* leading dimension of B */
                    (DTYPE)1.,                              /* beta */
                    paramGrads[0],                          /* C */
                    rowCount);                              /* leading dimension of C */

        /*for (size_t idx = 0; idx < elemCount; idx++)
            printf("paramGrads[%zu] = %f\n", idx, paramGrads[0][idx]);*/

#if USE_ICCL
        op->GetParameterSet(0)->StartGradientComm(paramGrads[0]);
#endif

        if (inputGrads[0])
        {
            MY_ASSERT(params[0], "params are null");

            cblas_sgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
                        LOCAL_MINIBATCH_SIZE,                   /* number of rows in A and C */
                        CLASS_COUNT,                            /* number of columns in B and C */
                        CLASS_COUNT,                            /* number of columns in A and number of rows in B */
                        (DTYPE)1.,                              /* alfa */
                        outputGrads[0],                         /* A */
                        CLASS_COUNT,                            /* leading dimension of A */
                        params[0],                              /* B */
                        rowCount,                               /* leading dimension of B */
                        (DTYPE)0.,                              /* beta */
                        inputGrads[0],                          /* C */
                        rowCount);                              /* leading dimension of C */
        }
    }

    void Update()
    {
#if USE_ICCL
        DTYPE* commBuf = (DTYPE*)op->GetParameterSet(0)->WaitGradientComm();
#else
        DTYPE* commBuf = NULL;
#endif

        if (commBuf)
        {
            MY_ASSERT(commBuf == paramGrads[0], "commBuf should be equal to paramGrads[0]");
            /*for (size_t idx = 0; idx < elemCount; idx++)
                printf("commBuf[%zu] = %f\n", idx, commBuf[idx]);*/
        }

        ApplyUpdate();
    }

    void ApplyUpdate()
    {
        /* perform SGD step and update weights */
        DTYPE lr = BASE_LR;

        /* normalize */
#if USE_ICCL
        if (processCount > 1)
        {
            const DTYPE normCoef = DTYPE(1.) / processCount;
            /* x = a * x */
            cblas_sscal(elemCount,     /* number of elements in x */
                        normCoef,      /* a */
                        paramGrads[0], /* x */
                        1);            /* increment for elements of x */
        }
#endif

        /* regularize */
        DTYPE localDecay = 0.00001;

        /* y := a * x + y */
        cblas_saxpy(elemCount,     /* number of elements in x and y */
                    localDecay,    /* a */
                    params[0],     /* x */
                    1,             /* increment for elements of x */
                    paramGrads[0], /* y */
                    1);            /* increment for elements of y */

        /* compute updated value */
        DTYPE localRate = lr * 0.2;
        DTYPE momentum = 1.0;

        /* y := a * x + b * y */
        cblas_saxpby(elemCount,     /* number of elements in x and y */
                     localRate,     /* a */
                     paramGrads[0], /* x */
                     1,             /* increment for elements of x */
                     momentum,      /* b */
                     history,       /* y */
                     1);            /* increment for elements of x */
        memcpy(paramGrads[0], history, paramSize);

        /* apply update */
        cblas_saxpy(elemCount,     /* number of elements in x and y */
                    DTYPE(-1),     /* a */
                    paramGrads[0], /* x */
                    1,             /* increment for elements of x */
                    params[0],     /* y */
                    1);            /* increment for elements of y */
    }
};

#endif /* INNER_PRODUCT_LAYER_HPP */
