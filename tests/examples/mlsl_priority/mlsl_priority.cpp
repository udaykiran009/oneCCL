#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <sys/time.h>
#include <inttypes.h>
#include <unistd.h>

#include "mlsl.hpp"

size_t compIterTimeMs = 0; // 1900 for VGG-16 with BS=64, 970 for Resnet-50 with BS=64, 950 for GN V3

/* VGG16 in backprop order */
/*size_t msgSizes[] = { 16384000, 4000, 67108864, 16384, 411041792, 16384, 9437184, 2048, 9437184, 2048, 9437184, 2048,
                      9437184, 2048, 9437184, 2048, 4718592, 2048, 2359296, 1024, 2359296, 1024, 1179648, 1024, 589824,
                      512, 294912, 512, 147456, 256, 6912, 256 };*/

/* Alexnet in backprop order */
/*size_t msgSizes[] = { 16384000, 4000, 67108864, 16384, 150994944, 16384, 1769472, 1024, 2654208,
                        1536, 3538944, 1536, 1228800, 1024, 139392, 384 };*/

/* Resnet50 in backprop order */
/*size_t msgSizes[] = { 8192000, 4000, 4194304, 9437184, 4194304, 4194304, 9437184, 4194304, 4194304, 9437184,
                        2097152, 8388608, 1048576, 2359296, 1048576, 1048576, 2359296, 1048576, 1048576, 2359296,
                        1048576, 1048576, 2359296, 1048576, 1048576, 2359296, 1048576, 1048576, 2359296, 524288,
                        2097152, 262144, 589824, 262144, 262144, 589824, 262144, 262144, 589824, 262144, 262144,
                        589824, 131072, 524288, 65536, 147456, 65536, 65536, 147456, 65536, 65536, 147456, 16384,
                        65536, 37632, 256 };*/

/* VGG19 in backprop order */
/*size_t msgSizes[] = { 16384000, 4000, 67108864, 16384, 411041792, 16384, 9437184, 2048, 9437184, 2048, 9437184, 2048,
                        9437184, 2048, 9437184, 2048, 9437184, 2048, 9437184, 2048, 4718592, 2048, 2359296, 1024, 2359296,
                        1024, 2359296, 1024, 1179648, 1024, 589824, 512, 294912, 512, 147456, 256, 6912, 256 };*/

/* GN V3 in backprop order */
size_t msgSizes[] = { 8192000, 4000, 768, 768, 1572864, 1536, 1536, 1769472, 1536, 1536, 1769472, 1536, 1536, 6193152, 1792,
                       1792, 3670016, 1536, 1536, 1769472, 1536, 1536, 1769472, 1536, 1536, 3145728, 1280, 1280, 2621440, 768,
                       768, 983040, 1536, 1536, 1769472, 1536, 1536, 1769472, 1536, 1536, 6193152, 1792, 1792, 2293760, 1536,
                       1536, 1769472, 1536, 1536, 1769472, 1536, 1536, 1966080, 1280, 1280, 1638400, 768, 768, 1327104, 768,
                       768, 1032192, 768, 768, 1032192, 768, 768, 589824, 1280, 1280, 2211840, 768, 768, 589824, 768, 768,
                       589824, 768, 768, 1032192, 768, 768, 1032192, 768, 768, 1032192, 768, 768, 1032192, 768, 768, 589824,
                       768, 768, 1032192, 768, 768, 1032192, 768, 768, 589824, 768, 768, 589824, 768, 768, 589824, 768, 768,
                       860160, 640, 640, 716800, 640, 640, 716800, 640, 640, 716800, 640, 640, 491520, 768, 768, 860160, 640,
                       640, 716800, 640, 640, 491520, 768, 768, 589824, 768, 768, 589824, 768, 768, 860160, 640, 640, 716800,
                       640, 640, 716800, 640, 640, 716800, 640, 640, 491520, 768, 768, 860160, 640, 640, 716800, 640, 640, 491520,
                       768, 768, 589824, 768, 768, 589824, 768, 768, 688128, 512, 512, 458752, 512, 512, 458752, 512, 512, 458752,
                       512, 512, 393216, 768, 768, 688128, 512, 512, 458752, 512, 512, 393216, 768, 768, 589824, 384, 384, 331776,
                       384, 384, 221184, 256, 256, 73728, 1536, 1536, 3981312, 256, 256, 73728, 384, 384, 331776, 384, 384, 221184,
                       256, 256, 73728, 256, 256, 307200, 192, 192, 55296, 256, 256, 73728, 256, 256, 65536, 384, 384, 331776, 384,
                       384, 221184, 256, 256, 65536, 256, 256, 307200, 192, 192, 49152, 256, 256, 65536, 128, 128, 24576, 384, 384,
                       331776, 384, 384, 221184, 256, 256, 49152, 256, 256, 307200, 192, 192, 36864, 256, 256, 49152, 768, 768,
                       552960, 320, 320, 20480, 256, 256, 73728, 128, 128, 36864, 128, 128, 3456 };

using namespace MLSL;

#define sizeofa(arr)      (sizeof(arr) / sizeof(*arr))
#define DTYPE             float
#define DTYPE_SIZE        sizeof(DTYPE)
#define MLSL_DTYPE        ((DTYPE_SIZE == 4) ? DT_FLOAT : DT_DOUBLE)
#define CACHELINE_SIZE    64

#define MAX_MSG_COUNT     512
#define ITER_COUNT        40
#define WARMUP_ITER_COUNT 10

size_t processIdx;
size_t processCount;

size_t totalMsgCount;
void* msgBuffers[MAX_MSG_COUNT];
int msgCompletions[MAX_MSG_COUNT];

double tmpStartTimer, tmpStopTimer;
double iterStart, iterStop, iterTimer, iterIsoTimer;

double msgStarts[MAX_MSG_COUNT];
double msgStops[MAX_MSG_COUNT];
double msgTimers[MAX_MSG_COUNT];
double msgIsoTimers[MAX_MSG_COUNT];

#if 0
double msgWithPrevStarts[MAX_MSG_COUNT][MAX_MSG_COUNT];
double msgWithPrevStops[MAX_MSG_COUNT][MAX_MSG_COUNT];
double msgWithPrevTimers[MAX_MSG_COUNT][MAX_MSG_COUNT];
#endif

double msgPureStartTimers[MAX_MSG_COUNT];
double msgPureWaitTimers[MAX_MSG_COUNT];

Operation* op;
Distribution* distribution;
size_t compDelayMs;

static double when(void)
{
    struct timeval tv;
    static struct timeval tv0;
    static int first = 1;
    int err;

    err = gettimeofday(&tv, NULL);
    if (err) {
        perror("gettimeofday");
        return 0;
    }

    if (first)
    {
        tv0 = tv;
        first = 0;
    }
    return (double)(tv.tv_sec - tv0.tv_sec) * 1.0e6 + (double)(tv.tv_usec - tv0.tv_usec); 
}

void doIter(size_t iterIdx)
{
    size_t idx, msgIdx, paramSetIdx;
    distribution->Barrier(GT_GLOBAL);

    iterStart = when();
    for (idx = 0; idx < totalMsgCount; idx++)
    {
        paramSetIdx = (totalMsgCount - idx - 1);
        msgIdx = idx;
        tmpStartTimer = when();
        op->GetParameterSet(paramSetIdx)->StartGradientComm(msgBuffers[msgIdx]);
        op->GetParameterSet(paramSetIdx)->WaitGradientComm();
        tmpStopTimer = when();
        msgIsoTimers[msgIdx] += (tmpStopTimer - tmpStartTimer);
    }
    iterStop = when();
    iterIsoTimer += (iterStop - iterStart);

    distribution->Barrier(GT_GLOBAL);

#if 0
    for (idx = 0; idx < totalMsgCount; idx++)
    {
        paramSetIdx = (totalMsgCount - idx - 1);
        msgIdx = idx;

        size_t maxPrevCount = msgIdx;
        // if (processIdx == 0)
        // printf("msg with size %zu can have up to %zu prevs\n", msgSizes[msgIdx], maxPrevCount);
        if (maxPrevCount == 0) continue;

        for (size_t prevCount = maxPrevCount; prevCount >= 1; prevCount--)
        {
            distribution->Barrier(GT_GLOBAL);

            for (size_t prevIdx = prevCount; prevIdx >= 1; prevIdx--)
            {
                size_t prevParamSetIdx = paramSetIdx + prevIdx;
                size_t prevMsgIdx = msgIdx - prevIdx;
                if (processIdx == 0 && prevCount == 26)
                printf("(%zu): start prev with size %zu\n", msgSizes[msgIdx], msgSizes[prevMsgIdx]);
                op->GetParameterSet(prevParamSetIdx)->StartGradientComm(msgBuffers[prevMsgIdx]);
            }

            msgWithPrevStarts[msgIdx][prevCount] = when();
            // if (processIdx == 0)
            // printf("(%zu): start and wait main msg\n", msgSizes[msgIdx]);
            op->GetParameterSet(paramSetIdx)->StartGradientComm(msgBuffers[msgIdx]);
            op->GetParameterSet(paramSetIdx)->WaitGradientComm();
            msgWithPrevStops[msgIdx][prevCount] = when();
            msgWithPrevTimers[msgIdx][prevCount] += (msgWithPrevStops[msgIdx][prevCount] - msgWithPrevStarts[msgIdx][prevCount]);

            for (size_t prevIdx = 1; prevIdx <= prevCount ; prevIdx++)
            {
                size_t prevParamSetIdx = paramSetIdx + prevIdx;
                size_t prevMsgIdx = msgIdx - prevIdx;
                // if (processIdx == 0)
                // printf("(%zu): wait prev with size %zu\n", msgSizes[msgIdx], msgSizes[prevMsgIdx]);
                op->GetParameterSet(prevParamSetIdx)->WaitGradientComm();
            }
        }
    }
    distribution->Barrier(GT_GLOBAL);
#endif

    memset(msgCompletions, 0, MAX_MSG_COUNT * sizeof(int));
    size_t completions = 0;

    iterStart = when();
    for (idx = 0; idx < totalMsgCount; idx++)
    {
        paramSetIdx = (totalMsgCount - idx - 1);
        msgIdx = idx;

        if (idx % 2 == 0) usleep(compDelayMs * 1000);

        msgStarts[msgIdx] = when();
        tmpStartTimer = when();
        op->GetParameterSet(paramSetIdx)->StartGradientComm(msgBuffers[msgIdx]);
        tmpStopTimer = when();
        msgPureStartTimers[msgIdx] += (tmpStopTimer - tmpStartTimer);
    }

    while (completions < totalMsgCount)
    {
        for (idx = 0; idx < totalMsgCount; idx++)
        {
            paramSetIdx = idx;
            msgIdx = (totalMsgCount - idx - 1);

            if (msgCompletions[msgIdx]) continue;

            bool isCompleted = false;

            //op->GetParameterSet(paramSetIdx)->TestGradientComm(&isCompleted);

            tmpStartTimer = when();
            op->GetParameterSet(paramSetIdx)->WaitGradientComm(); isCompleted = true;
            tmpStopTimer = when();
            msgPureWaitTimers[msgIdx] += (tmpStopTimer - tmpStartTimer);

            if (isCompleted)
            {
                msgStops[msgIdx] = when();
                msgTimers[msgIdx] += (msgStops[msgIdx] - msgStarts[msgIdx]);
                msgCompletions[msgIdx] = 1;
                completions++;
            }
        }
    }
    iterStop = when();
    iterTimer += (iterStop - iterStart);
}

int main(int argc, char** argv)
{
    Environment::GetEnv().Init(&argc, &argv);
    Session* session = Environment::GetEnv().CreateSession();
    processCount = Environment::GetEnv().GetProcessCount();
    session->SetGlobalMinibatchSize(processCount);
    processIdx = Environment::GetEnv().GetProcessIdx();
    distribution = Environment::GetEnv().CreateDistribution(processCount, 1);
    OperationRegInfo* regInfo = session->CreateOperationRegInfo(OT_CC);

    totalMsgCount = sizeofa(msgSizes);
    assert(totalMsgCount <= MAX_MSG_COUNT);

    char* compIterTimeMsEnv = getenv("COMP_ITER_TIME_MS");
    if (compIterTimeMsEnv)
    {
        compIterTimeMs = atoi(compIterTimeMsEnv);
    }
    compDelayMs = 2 * compIterTimeMs / totalMsgCount;

    size_t totalMsgSize = 0;
    size_t idx, msgIdx;
    for (idx = 0; idx < totalMsgCount; idx++)
    {
        totalMsgSize += msgSizes[idx];
    }

    for (idx = 0; idx < totalMsgCount; idx++)
    {
        msgIdx = (totalMsgCount - idx - 1);
        /* add param_set with higher priority firstly */
        regInfo->AddParameterSet(msgSizes[msgIdx] / 4, 1, MLSL_DTYPE);
    }

    size_t opIdx = session->AddOperation(regInfo, distribution);
    session->DeleteOperationRegInfo(regInfo);
    op = session->GetOperation(opIdx);
    session->Commit();

    assert(totalMsgCount == op->GetParameterSetCount());

    if (processIdx == 0)
    {
        printf("iterCount: %d, warmupIterCount: %d\n", ITER_COUNT, WARMUP_ITER_COUNT);
        printf("totalMsgCount: %zu, totalMsgSize: %zu bytes\n", totalMsgCount, totalMsgSize);
        printf("compIterTimeMs: %zu, compDelayMs: %zu (between each pair of messages)\n", compIterTimeMs, compDelayMs);
        printf("messages are started in direct order and completed in reverse order\n");
        fflush(stdout);
    }

    for (idx = 0; idx < totalMsgCount; idx++)
    {
        size_t msgSize = msgSizes[idx];
        msgBuffers[idx] = Environment::GetEnv().Alloc(msgSize, CACHELINE_SIZE);
        memset(msgBuffers[idx], 'a' + idx, msgSize);
    }

    /* warmup */
    for (size_t iter = 0; iter < WARMUP_ITER_COUNT; iter++)
    {
        doIter(iter);
    }

    /* reset timers */
    iterStart = iterStop = iterTimer = iterIsoTimer = 0;
    for (idx = 0; idx < totalMsgCount; idx++)
    {
        msgStarts[idx] = msgStops[idx] = msgTimers[idx] = msgIsoTimers[idx] =
            msgPureStartTimers[idx] = msgPureWaitTimers[idx] = 0;

#if 0
        for (size_t prevIdx = 0; prevIdx < totalMsgCount; prevIdx++)
        {
            msgWithPrevStarts[idx][prevIdx] =
            msgWithPrevStops[idx][prevIdx] =
            msgWithPrevTimers[idx][prevIdx] = 0;
        }
#endif
    }

    /* main loop */
    for (size_t iter = 0; iter < ITER_COUNT; iter++)
    {
        doIter(iter);
    }

    if (processIdx == 0)
    {
        printf("-------------------------------------------------------------------------------------------------------------\n");
        printf("msg_idx | size (bytes) | msg_time (usec) | msg_start_time (usec) | msg_wait_time (usec) | msg_iso_time (usec)\n");
        printf("-------------------------------------------------------------------------------------------------------------\n");
    }

    for (idx = 0; idx < totalMsgCount; idx++)
    {
        Environment::GetEnv().Free(msgBuffers[idx]);
        if (processIdx == 0)
        {
            printf("%7zu | %12zu | %15.2lf | %21.2lf | %20.2lf | %19.2lf ",
                   idx, msgSizes[idx], msgTimers[idx] / ITER_COUNT, msgPureStartTimers[idx] / ITER_COUNT, msgPureWaitTimers[idx] / ITER_COUNT, msgIsoTimers[idx] / ITER_COUNT);

#if 0
            for (size_t prevIdx = 0; prevIdx < totalMsgCount; prevIdx++)
            {
                if (msgWithPrevTimers[idx][prevIdx] == 0) continue;
                printf("| %2d: %6.2lf", prevIdx, msgWithPrevTimers[idx][prevIdx] / ITER_COUNT);
            }
#endif
            printf("\n");
        }
    }

    if (processIdx == 0)
    {
        printf("-------------------------------------------------------------------------------------------------------------\n");
        printf("iter_time     (usec): %12.2lf\n", iterTimer / ITER_COUNT);
        printf("iter_iso_time (usec): %12.2lf\n", iterIsoTimer / ITER_COUNT);
    }

    Environment::GetEnv().DeleteSession(session);
    Environment::GetEnv().DeleteDistribution(distribution);
    Environment::GetEnv().Finalize();

    return 0;
}
