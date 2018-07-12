
/* MLSL library API test for multiple sequential communications */

#include <stdio.h>  /* printf */
#include <stdlib.h> /* exit */
#include <unistd.h> /* usleep */

#include "mlsl.hpp"

using namespace MLSL;

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

#define DTYPE          float
#define DTYPE_SIZE     sizeof(DTYPE)
#define MLSL_DTYPE     ((DTYPE_SIZE == 4) ? DT_FLOAT : DT_DOUBLE)
#define CACHELINE_SIZE 64
#define ELEM_COUNT     32768
#define ITER_COUNT     1000

int main(int argc, char** argv)
{
    if (argc != 1)
    {
        printf("specify parameters: mlsl_multiple_comms\n");
        exit(0);
    }

    Environment::GetEnv().Init(&argc, &argv);
    size_t processCount = Environment::GetEnv().GetProcessCount();
    size_t processIdx = Environment::GetEnv().GetProcessIdx();
    printf("process_idx %zu\n", processIdx);
    Distribution* dist = Environment::GetEnv().CreateDistribution(processCount / 4, 4);
    DTYPE* sendBuf = (DTYPE*)Environment::GetEnv().Alloc(ELEM_COUNT * DTYPE_SIZE, CACHELINE_SIZE);
    DTYPE* recvBuf = (DTYPE*)Environment::GetEnv().Alloc(ELEM_COUNT * DTYPE_SIZE * processCount, CACHELINE_SIZE);

    for (size_t idx = 0; idx < ITER_COUNT; idx++)
    {
        CommReq* req;

        req = dist->AllReduce(sendBuf, sendBuf, ELEM_COUNT, MLSL_DTYPE, RT_SUM, GT_GLOBAL);
        Environment::GetEnv().Wait(req);

        req = dist->Gather(sendBuf, ELEM_COUNT, recvBuf, MLSL_DTYPE, 0, GT_MODEL);
        Environment::GetEnv().Wait(req);
    }

    Environment::GetEnv().Free(sendBuf);
    Environment::GetEnv().Free(recvBuf);
    Environment::GetEnv().DeleteDistribution(dist);
    Environment::GetEnv().Finalize();
    printf("[%zu] exited normally\n", processIdx);
    return 0;
}
