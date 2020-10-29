#ifndef COMMON_HELPERS_H
#define COMMON_HELPERS_H

#ifdef HOST_CTX
#define __global
using namespace ccl;

#include <cstdint>

#else

// Define aliases for OpenCL types
typedef char    int8_t;
typedef uchar   uint8_t;
typedef short   int16_t;
typedef ushort  uint16_t;
typedef int     int32_t;
typedef uint    uint32_t;
typedef long    int64_t;
typedef ulong   uint64_t;
typedef half    float16_t;
typedef float   float32_t;
typedef double  float64_t;
typedef ushort  bfloat16_t;

typedef ushort  bfloat16;

#endif

#endif /* COMMON_HELPERS_H */
