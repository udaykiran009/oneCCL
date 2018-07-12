#ifndef COMMON_HPP
#define COMMON_HPP

#if ENABLE_CHKP
#include <chkp.h>
#endif

#if defined(__INTEL_COMPILER) || defined(__ICC)
#include <mm_malloc.h>
#endif
#include <stdlib.h>

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))

#if defined(__INTEL_COMPILER) || defined(__ICC)
#define MLSL_MALLOC_IMPL(size, align) _mm_malloc(size, align)
#define MLSL_FREE_IMPL(ptr)           _mm_free(ptr)
#elif defined(__GNUC__)
#define MLSL_MALLOC_IMPL(size, align) ({ void* ptr; int pm_ret __attribute__((unused)) = posix_memalign((void**)(&ptr), align, size); ptr; })
#define MLSL_FREE_IMPL(ptr)           free(ptr)
#else
# error "this compiler is not supported" 
#endif


#if ENABLE_CHKP
#define MAKE_BOUNDS(ptr, size) { ptr = __chkp_make_bounds(ptr, size); }
#define MLSL_MALLOC(size, align) ({ void* ptr = MLSL_MALLOC_IMPL(size, align); MAKE_BOUNDS(ptr, size); ptr; })
#else
#define MAKE_BOUNDS(ptr, size)
#define MLSL_MALLOC(size, align) MLSL_MALLOC_IMPL(size, align)
#endif

#define MLSL_FREE(ptr) MLSL_FREE_IMPL(ptr)


#if defined(__INTEL_COMPILER) || defined(__ICC)
static inline unsigned long long rdtsc()
{
    return __rdtsc();
}
#elif defined(__GNUC__)
static inline unsigned long long rdtsc()
{
    unsigned int lo,hi;
    __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
    return ((unsigned long long)hi << 32) | lo;
}
#else
# error "this compiler is not supported" 
#endif

#define CHECK_RANGE(value, start, end) ((size_t)value >= start && (size_t)value < end && (int)value >= (int)start && (int)value < (int)end)

#define CACHELINE_SIZE 64
#define ONE_MB         1048576
#define TWO_MB         2097152

#endif /* COMMON_HPP */
