#if USE_MLSL

#include "mlsl_wrapper.hpp"

Session* session;
Distribution* distribution;
size_t processIdx;
size_t processCount;

void InitMLSL(int argc, char** argv)
{
    Environment::GetEnv().Init(&argc, &argv);
    session = Environment::GetEnv().CreateSession();
    processCount = Environment::GetEnv().GetProcessCount();
    session->SetGlobalMinibatchSize(LOCAL_MINIBATCH_SIZE * processCount);

    size_t modelParts = 1;
    size_t dataParts = processCount/modelParts;
    distribution = Environment::GetEnv().CreateDistribution(dataParts, modelParts);

    processIdx = Environment::GetEnv().GetProcessIdx();
    if (processIdx == 0)
        printf("process_count = %zu, distribution = %zu x %zu (data_parts x model_parts)\n", processCount, dataParts, modelParts);
}

void FinalizeMLSL()
{
    Environment::GetEnv().DeleteSession(session);
    Environment::GetEnv().DeleteDistribution(distribution);
    Environment::GetEnv().Finalize();
}

void Bcast(void* buffer, size_t count)
{
    CommReq* req = distribution->Bcast(buffer, count, MLSL_DTYPE,
                                       0, GT_GLOBAL);
    Environment::GetEnv().Wait(req);
}

#endif
