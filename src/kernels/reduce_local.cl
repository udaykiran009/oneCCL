#include "common.h"
#include "shared.h"

#define DEFINE_KERNEL(Name, T, VecSize, Op, OpName) \
    __kernel void reduce_local_inplace_execution_##Name##_##OpName( \
        ulong count, const __global T* input_buffer, __global T* inoutput_buffer) { \
        DEBUG_BLOCK(/* int sg_id = get_sub_group_id(); */ \
                    printf("in reduce_local_inplace_execution_\n")); \
        size_t work_group_size = get_global_size(0); \
        size_t thread_id = get_global_id(0); \
\
        DEBUG_BLOCK(/* int sg_id = get_sub_group_id(); */ \
                    printf("in reduce_local_inplace_execution_: before for\n")); \
        for (size_t i = 0; thread_id + i < count; i += work_group_size) { \
            const size_t idx = thread_id + i; \
            inoutput_buffer[idx] = Op(input_buffer[idx], inoutput_buffer[idx]); \
        } \
        DEBUG_BLOCK(/* int sg_id = get_sub_group_id(); */ \
                    printf("in reduce_local_inplace_execution_: after for\n")); \
    }

// Define kernels for a specific operation for all the supported types.
// Note: for op function we use convention __<OpName>_<type>, where type is the actual type(e.g. int4, float)
// FIXME: Temporary use scalar types instead of vector ones. This is a workaround for issues in case when
// elems_count % VecSize != 0. Need to find a proper fix with a good performance.
#define VEC_SIZE RING_ALLREDUCE_VEC_SIZE

#define DEFINE_KERNELS_WITH_OP(OpName) \
    DEFINE_KERNEL(int8, char, VEC_SIZE, __##OpName##_##char, OpName) \
    DEFINE_KERNEL(uint8, uchar, VEC_SIZE, __##OpName##_##uchar, OpName) \
\
    DEFINE_KERNEL(int16, short, VEC_SIZE, __##OpName##_##short, OpName) \
    DEFINE_KERNEL(uint16, ushort, VEC_SIZE, __##OpName##_##ushort, OpName) \
\
    DEFINE_KERNEL(int32, int, VEC_SIZE, __##OpName##_##int, OpName) \
    DEFINE_KERNEL(uint32, uint, VEC_SIZE, __##OpName##_##uint, OpName) \
\
    DEFINE_KERNEL(int64, long, VEC_SIZE, __##OpName##_##long, OpName) \
    DEFINE_KERNEL(uint64, ulong, VEC_SIZE, __##OpName##_##ulong, OpName) \
\
    DEFINE_KERNEL(float32, float, VEC_SIZE, __##OpName##_##float, OpName) \
    DEFINE_KERNEL(float64, double, VEC_SIZE, __##OpName##_##double, OpName)

#define DEFINE_KERNELS_WITH_LP_OP(OpName) \
    DEFINE_KERNEL(bfloat16, ushort, VEC_SIZE, __bf16_##OpName##_##ushort, OpName) \
    DEFINE_KERNEL(float16, half, VEC_SIZE, __##OpName##_##half, OpName)

#define DEFINE_OPS(T) \
    DEFINE_SUM_OP(T) \
    DEFINE_PROD_OP(T) \
    DEFINE_MIN_OP(T) \
    DEFINE_MAX_OP(T)

#define DEFINE_BF16OPS(T) \
    DEFINE_BF16SUM_OP(T) \
    DEFINE_BF16PROD_OP(T) \
    DEFINE_BF16MIN_OP(T) \
    DEFINE_BF16MAX_OP(T)

#define DEFINE_FP16OPS(T) \
    DEFINE_FP16SUM_OP(T) \
    DEFINE_FP16PROD_OP(T) \
    DEFINE_FP16MIN_OP(T) \
    DEFINE_FP16MAX_OP(T)

// Define Op function for each supported type(use vector types for some of them as required by the kernel)
DEFINE_OPS(char)
DEFINE_OPS(uchar)

DEFINE_OPS(short)
DEFINE_OPS(ushort)

DEFINE_OPS(int)
DEFINE_OPS(uint)

DEFINE_OPS(long)
DEFINE_OPS(ulong)

DEFINE_OPS(float)
DEFINE_OPS(double)

DEFINE_BF16OPS(ushort)
DEFINE_FP16OPS(half)

// Define the actual kernels
DEFINE_KERNELS_WITH_OP(sum)
DEFINE_KERNELS_WITH_OP(prod)
DEFINE_KERNELS_WITH_OP(min)
DEFINE_KERNELS_WITH_OP(max)

DEFINE_KERNELS_WITH_LP_OP(sum)
DEFINE_KERNELS_WITH_LP_OP(prod)
DEFINE_KERNELS_WITH_LP_OP(min)
DEFINE_KERNELS_WITH_LP_OP(max)
