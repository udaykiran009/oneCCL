#pragma once

#pragma OPENCL EXTENSION cl_intel_subgroups : enable
#pragma OPENCL EXTENSION cl_khr_subgroups : enable

#include "lp.h"

#define FORMAT_int8_t  "%hhd"
#define FORMAT_int16_t "%d"
#define FORMAT_int32_t "%d"
#define FORMAT_int64_t "%ld"

#define FORMAT_uint8_t  "%hhu"
#define FORMAT_uint16_t "%u"
#define FORMAT_uint32_t "%u"
#define FORMAT_uint64_t "%lu"

#define FORMAT_float  "%f"
#define FORMAT_double "%f"

#define FORMAT_ushort "%u"
#define FORMAT_half   "%f"

#define FORMAT_4(format) #format ", " #format ", " #format ", " #format
#define FORMAT_char4     FORMAT_4(% hhd)
#define FORMAT_uchar4    FORMAT_4(% hhu)
#define FORMAT_short4    FORMAT_4(% d)
#define FORMAT_ushort4   FORMAT_4(% u)
#define FORMAT_int4      FORMAT_4(% d)
#define FORMAT_uint4     FORMAT_4(% u)
#define FORMAT_long4     FORMAT_4(% ld)
#define FORMAT_ulong4    FORMAT_4(% lu)
#define FORMAT_float4    FORMAT_4(% f)
#define FORMAT_double4   FORMAT_4(% f)

#ifdef ENABLE_KERNEL_DEBUG
#define DEBUG_BLOCK(block) block
#else
#define DEBUG_BLOCK(block)
#endif

#define DEFINE_SUM_OP(T) \
    T __sum_##T(T lhs, T rhs) { \
        return lhs + rhs; \
    }

#define DEFINE_PROD_OP(T) \
    T __prod_##T(T lhs, T rhs) { \
        return lhs * rhs; \
    }

#define DEFINE_MIN_OP(T) \
    T __min_##T(T lhs, T rhs) { \
        return min(lhs, rhs); \
    }

#define DEFINE_MAX_OP(T) \
    T __max_##T(T lhs, T rhs) { \
        return max(lhs, rhs); \
    }
