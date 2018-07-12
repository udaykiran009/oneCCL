#ifndef COMMON_HPP
#define COMMON_HPP

#include <cstdio>  /* printf */
#include <cstdlib> /* malloc */

#if USE_MLSL
#include "mlsl_wrapper.hpp"
#endif

#define DTYPE                 float
#define DTYPE_SIZE            sizeof(DTYPE)
#define MLSL_DTYPE            ((DTYPE_SIZE == 4) ? DT_FLOAT : DT_DOUBLE)
#define CACHELINE_SIZE        64
#define LOCAL_MINIBATCH_SIZE  2
#define IMAGE_SIZE            2
#define CLASS_COUNT           2
#define HIDDEN_LAYER_COUNT    4
#define LAYER_COUNT           (HIDDEN_LAYER_COUNT + 2) // data layer, hidden layers and softmax layer
#define EPOCH_COUNT           4
#define MINIBATCH_PER_EPOCH   4

/* https://github.com/beniz/deepdetect/blob/master/templates/caffe/mlp/mlp_solver.prototxt */
#define BASE_LR               (0.1)
#define MOMENTUM              (0.9)
#define LR_POLICY             "fixed"

#if USE_MLSL
#define MY_MALLOC(size) Environment::GetEnv().Alloc(size, CACHELINE_SIZE)
#define MY_FREE(ptr)    Environment::GetEnv().Free(ptr)
#else
#define MY_MALLOC(size) malloc(size)
#define MY_FREE(ptr)    free(ptr)
#endif

#define MY_ASSERT(cond,...)                                                   \
  do                                                                          \
  {                                                                           \
      if (!(cond))                                                            \
      {                                                                       \
          printf("%s:%d:assertion '%s' failed\n", __FILE__, __LINE__, #cond); \
          exit(1);                                                            \
      }                                                                       \
  } while(0)

#endif /* COMMON_HPP */
