#ifndef COMMON_HELPERS_H
#define COMMON_HELPERS_H

#ifdef HOST_CTX
#define __global
using namespace ccl;

#include <cstdint>

#else

// Define aliases for OpenCL types
// typedef char    int8;
// typedef uchar   uint8;
// typedef short   int16;
// typedef ushort  uint16;
// typedef int     int32;
// typedef uint    uint32;
// typedef long    int64;
// typedef ulong   uint64;
// typedef half    float16;
// typedef float   float32;
// typedef double  float64;
typedef ushort  bfloat16;

#endif

#endif /* COMMON_HELPERS_H */
