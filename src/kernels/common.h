#pragma once

#ifdef HOST_CTX

#define __global
using namespace ccl;
#include <cstdint>

#else /* HOST_CTX */

#pragma OPENCL EXTENSION cl_intel_subgroups : enable
#pragma OPENCL EXTENSION cl_khr_subgroups : enable

#include "lp.h"

// Define aliases for OpenCL types
typedef char int8_t;
typedef uchar uint8_t;
typedef short int16_t;
typedef ushort uint16_t;
typedef int int32_t;
typedef uint uint32_t;
typedef long int64_t;
typedef ulong uint64_t;
typedef half float16_t;
typedef float float32_t;
typedef double float64_t;
typedef ushort bfloat16_t;
typedef ushort bfloat16;

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

#ifdef KERNEL_DEBUG
#define DEBUG_BLOCK(block) block
#else
#define DEBUG_BLOCK(block)
#endif

#ifdef KERNEL_DEBUG
#define LOG_INPUT_DATA_START(rank)    printf("kernel %zu, wait income data\n", rank)
#define LOG_INPUT_DATA_END(rank)      printf("kernel %zu, received data\n", rank)
#define LOG_OUTGOING_DATA_START(rank) printf("kernel %zu, wait signal to send\n", rank)
#define LOG_OUTGOING_DATA_END(rank)   printf("kernel %zu, received signal to send\n", rank)
#define LOG_SEND_PROGRESS(rank, thread_id, flag, desired) \
    printf("kernel %zu.%d, send %d/%d\n", rank, thread_id, flag, desired)
#define LOG_BARRIER_PASSED(rank, thread_id) \
    printf("kernel %zu.%d barrier passed\n", rank, thread_id);
#define LOG_IN_BARRIER(rank, thread_id, flag, desired) \
    printf("kernel %zu.%d barrier %d/%d\n", rank, thread_id, flag, desired);
#else /* KERNEL_DEBUG */
#define LOG_INPUT_DATA_START(rank)
#define LOG_INPUT_DATA_END(rank)
#define LOG_OUTGOING_DATA_START(rank)
#define LOG_OUTGOING_DATA_END(rank)
#define LOG_BARRIER_PASSED(rank, thread_id)
#define LOG_IN_BARRIER(rank, thread_id, flag, desired)
#endif /* KERNEL_DEBUG */

#define SWAP_VARIABLES(var1, var2, type) \
    do { \
        type tmp; \
        tmp = var1; \
        var1 = var2; \
        var2 = tmp; \
    } while (0);

int get_left_rank(int rank, int comm_size) {
    return rank == 0 ? comm_size - 1 : rank - 1;
}

int get_right_rank(int rank, int comm_size) {
    return rank == (comm_size - 1) ? 0 : rank + 1;
}

#define PUT_READY_TO_RECEIVE(_sync_flag) \
    if (thread_id == 0) { \
        (*_sync_flag)++; \
    }
/*
#define PUT_READY_TO_RECEIVE(_sync_flag)             \
    if(thread_id == 0)                               \
    {                                                \
        atomic_inc(_sync_flag);                      \
    }
*/

#define I_SENT(_sync_flag) \
    if (thread_id == 0) { \
        (*_sync_flag)++; \
    }
/*
#define I_SENT(_sync_flag)                          \
    if(thread_id == 0)                              \
    {                                               \
        atomic_inc(_sync_flag);                     \
    }
*/

#define WAIT_INPUT_DATA(_sync_flag, _desired) \
    if (thread_id == 0) { \
        LOG_INPUT_DATA_START(my_rank); \
        while (1) { \
            if (*_sync_flag == _desired) { \
                LOG_INPUT_DATA_END(my_rank); \
                ++_desired; \
                break; \
            } \
        } \
    }

/*
#define WAIT_INPUT_DATA(_sync_flag, _desired)                                       \
    if(thread_id == 0)                                                              \
    {                                                                               \
        LOG_INPUT_DATA_START(my_rank);                                              \
        while(1)                                                                    \
        {                                                                           \
            int _old_value = atomic_add(_sync_flag, 0);                             \
            if(_old_value == _desired)                                              \
            {                                                                       \
                LOG_INPUT_DATA_END(my_rank);                                        \
                ++_desired;                                                         \
                break;                                                              \
            }                                                                       \
        }                                                                           \
    }
*/

#define WAIT_SIGNAL_TO_SEND(_sync_flag, _desired) \
    if (thread_id == 0) { \
        LOG_OUTGOING_DATA_START(my_rank); \
        while (_desired != *_sync_flag) { \
        }; \
        LOG_OUTGOING_DATA_END(my_rank); \
        ++_desired; \
    }

/*
#define WAIT_SIGNAL_TO_SEND(_sync_flag, _desired)                                   \
    if(thread_id == 0)                                                              \
    {                                                                               \
        LOG_OUTGOING_DATA_START(my_rank);                                           \
        while(_desired != atomic_add(_sync_flag, 0)) {};                            \
        LOG_OUTGOING_DATA_END(my_rank);                                             \
        ++_desired;                                                                 \
    }
*/

/*
#define KERNEL_BARRIER(_barrier_flag, _desired, _increment)                         \
    do                                                                              \
    {                                                                               \
        int _barrier_value = atomic_add(_barrier_flag, 0);                          \
        atomic_inc(_barrier_flag);                                                  \
        int _old_value = _barrier_value;                                            \
        while(1)                                                                    \
        {                                                                           \
            / *thread that last reached the barrier will reset it                    \
              other threads may expect to receive _desired value while it can be 0  \
              check if received value is less than initially received* /             \
            if(_old_value == _desired || _old_value < _barrier_value)               \
            {                                                                       \
                BARRIER_PASSED(my_rank, thread_id);                                 \
                break;                                                              \
            }                                                                       \
            IN_BARRIER(my_rank, thread_id, _old_value, _desired);                   \
            _old_value = atomic_add(_barrier_flag, 0);                \
        }                                                                           \
    } while (0);
*/

/* for A2A */
/*#define WAIT_INPUT_DATA(_sync_flag, _desired) \
    if (local_thread_id == 0) { \
        LOG_INPUT_DATA_START(rank_id); \
        while (1) { \
            int _old_value = atomic_cmpxchg(_sync_flag, _desired, _desired); \
            if (_old_value == _desired) { \
                LOG_INPUT_DATA_END(rank_id); \
                _desired += 1 + comm_size; \
                break; \
            } \
        } \
    }

#define WAIT_SIGNAL_TO_SEND(_sync_flag, _desired) \
    if (local_thread_id == 0) { \
        LOG_OUTGOING_DATA_START(rank_id); \
        while (_desired != atomic_cmpxchg(_sync_flag, _desired, _desired)) { \
        }; \
        LOG_OUTGOING_DATA_END(rank_id); \
        _desired += comm_size; \
    }*/

#endif /* HOST_CTX */
