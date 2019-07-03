#pragma once

#if defined(__INTEL_COMPILER) || defined(__ICC)
#include <malloc.h>
#else

#include <mm_malloc.h>

#endif

#include <immintrin.h>
#include <stdlib.h>
#include <stddef.h>
#include <algorithm>
#include <chrono>
#include <time.h>

/* common */

#define ICCL_CALL(expr)                                \
  do {                                                 \
        status = (expr);                                 \
        ICCL_ASSERT(status == iccl_status_success,     \
            "bad status ", status);                    \
  } while (0)

#define unlikely(x_) __builtin_expect(!!(x_), 0)
#define likely(x_)   __builtin_expect(!!(x_), 1)

#ifndef container_of
#define container_of(ptr, type, field)                  \
    ((type *) ((char *)ptr - offsetof(type, field)))
#endif

#define CACHELINE_SIZE 64
#define ONE_MB         1048576
#define TWO_MB         2097152

/* Single-linked list */

struct iccl_slist_entry_t
{
    iccl_slist_entry_t* next;
};

struct iccl_slist_t
{
    iccl_slist_entry_t* head;
    iccl_slist_entry_t* tail;
};

/* malloc/realloc/free */

#if defined(__INTEL_COMPILER) || defined(__ICC)
#define ICCL_MEMALIGN_IMPL(size, align) _mm_malloc(size, align)
#define ICCL_REALLOC_IMPL(old_ptr, old_size, new_size, align)           \
      ({                                                                \
          void* new_ptr = NULL;                                         \
          if (!old_ptr) new_ptr = _mm_malloc(new_size, align);          \
          else if (!old_size) _mm_free(old_ptr);                        \
          else                                                          \
          {                                                             \
              new_ptr = _mm_malloc(new_size, align);                    \
              memcpy(new_ptr, old_ptr, std::min(old_size, new_size));   \
              _mm_free(old_ptr);                                        \
          }                                                             \
          new_ptr;                                                      \
      })
#define ICCL_CALLOC_IMPL(size, align)          \
      ({                                       \
          void* ptr = _mm_malloc(size, align); \
          memset(ptr, 0, size);                \
          ptr;                                 \
      })
#define ICCL_FREE_IMPL(ptr) _mm_free(ptr)
#elif defined(__GNUC__)
#define ICCL_MEMALIGN_IMPL(size, align)                                                 \
    ({                                                                                  \
      void* ptr = NULL;                                                                 \
      int pm_ret __attribute__((unused)) = posix_memalign((void**)(&ptr), align, size); \
      ptr;                                                                              \
    })
#define ICCL_REALLOC_IMPL(old_ptr, old_size, new_size, align) realloc(old_ptr, new_size)
#define ICCL_CALLOC_IMPL(size, align) calloc(size, 1)
#define ICCL_FREE_IMPL(ptr) free(ptr)
#else
# error "this compiler is not supported" 
#endif

#define ICCL_MEMALIGN_WRAPPER(size, align, name)                \
    ({                                                          \
        void *ptr = ICCL_MEMALIGN_IMPL(size, align);            \
        ICCL_THROW_IF_NOT(ptr, "ICCL Out of memory, ", name);   \
        ptr;                                                    \
    })

#define ICCL_REALLOC_WRAPPER(old_ptr, old_size, new_size, align, name)      \
    ({                                                                      \
        void *ptr = ICCL_REALLOC_IMPL(old_ptr, old_size, new_size, align);  \
        ICCL_THROW_IF_NOT(ptr, "ICCL Out of memory, ", name);               \
        ptr;                                                                \
    })

#define ICCL_CALLOC_WRAPPER(size, align, name)                  \
    ({                                                          \
        void *ptr = ICCL_CALLOC_IMPL(size, align);              \
        ICCL_THROW_IF_NOT(ptr, "ICCL Out of memory, ", name);   \
        ptr;                                                    \
    })

#define ICCL_MALLOC(size, name)                                ICCL_MEMALIGN_WRAPPER(size, CACHELINE_SIZE, name)
#define ICCL_MEMALIGN(size, align, name)                       ICCL_MEMALIGN_WRAPPER(size, align, name)
#define ICCL_CALLOC(size, name)                                ICCL_CALLOC_WRAPPER(size, CACHELINE_SIZE, name)
#define ICCL_REALLOC(old_ptr, old_size, new_size, align, name) ICCL_REALLOC_WRAPPER(old_ptr, old_size, new_size, align, name)
#define ICCL_FREE(ptr)                                         ICCL_FREE_IMPL(ptr)

/* other */

static inline size_t iccl_pof2(size_t number)
{
    size_t pof2 = 1;

    while (pof2 <= number)
    {
        pof2 <<= 1;
    }
    pof2 >>= 1;

    return pof2;
}

static inline size_t iccl_aligned_sz(size_t size,
                                     size_t alignment)
{
    return ((size % alignment) == 0) ?
           size : ((size / alignment) + 1) * alignment;
}

static inline timespec from_time_point(const std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds> point)
{
    auto sec = std::chrono::time_point_cast<std::chrono::seconds>(point);
    auto ns = std::chrono::time_point_cast<std::chrono::nanoseconds>(point) - std::chrono::time_point_cast<std::chrono::nanoseconds>(sec);

    return timespec { .tv_sec = sec.time_since_epoch().count(), .tv_nsec = ns.count() };
}
