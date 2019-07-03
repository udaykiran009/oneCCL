#ifndef ICCL_WRAPPER_HPP
#define ICCL_WRAPPER_HPP

#include "common.hpp"

#include "iccl.hpp"

using namespace ICCL;

extern Session* session;
extern Distribution* distribution;
extern size_t processIdx;
extern size_t processCount;

void InitICCL(int argc, char** argv);
void FinalizeICCL();
void Bcast(void* buffer, size_t count);

#endif /* ICCL_WRAPPER_HPP */
