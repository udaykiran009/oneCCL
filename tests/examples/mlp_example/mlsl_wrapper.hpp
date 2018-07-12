#ifndef MLSL_WRAPPER_HPP
#define MLSL_WRAPPER_HPP

#include "common.hpp"

#include "mlsl.hpp"

using namespace MLSL;

extern Session* session;
extern Distribution* distribution;
extern size_t processIdx;
extern size_t processCount;

void InitMLSL(int argc, char** argv);
void FinalizeMLSL();
void Bcast(void* buffer, size_t count);

#endif /* MLSL_WRAPPER_HPP */
