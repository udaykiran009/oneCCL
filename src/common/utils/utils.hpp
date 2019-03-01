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

/* common */

#define MLSL_CALL(expr)                              \
  do {                                               \
        status = expr;                               \
        MLSL_ASSERT(status == mlsl_status_success,   \
            "bad status %d", status);                \
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

struct mlsl_slist_entry_t
{
    mlsl_slist_entry_t* next;
};

struct mlsl_slist_t
{
    mlsl_slist_entry_t* head;
    mlsl_slist_entry_t* tail;
};

/* malloc/realloc/free */

#if defined(__INTEL_COMPILER) || defined(__ICC)
#define MLSL_MEMALIGN_IMPL(size, align) _mm_malloc(size, align)
#define MLSL_REALLOC_IMPL(old_ptr, old_size, new_size, align)           \
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
#define MLSL_CALLOC_IMPL(size, align)          \
      ({                                       \
          void* ptr = _mm_malloc(size, align); \
          memset(ptr, 0, size);                \
          ptr;                                 \
      })
#define MLSL_FREE_IMPL(ptr) _mm_free(ptr)
#elif defined(__GNUC__)
#define MLSL_MEMALIGN_IMPL(size, align)                                                 \
    ({                                                                                  \
      void* ptr = NULL;                                                                 \
      int pm_ret __attribute__((unused)) = posix_memalign((void**)(&ptr), align, size); \
      ptr;                                                                              \
    })
#define MLSL_REALLOC_IMPL(old_ptr, old_size, new_size, align) realloc(old_ptr, new_size)
#define MLSL_CALLOC_IMPL(size, align) calloc(size, 1)
#define MLSL_FREE_IMPL(ptr) free(ptr)
#else
# error "this compiler is not supported" 
#endif

#define MLSL_MEMALIGN_WRAPPER(size, align, name)                \
    ({                                                          \
        void *ptr = MLSL_MEMALIGN_IMPL(size, align);            \
        MLSL_THROW_IF_NOT(ptr, "MLSL Out of memory, %s", name); \
        ptr;                                                    \
    })

#define MLSL_REALLOC_WRAPPER(old_ptr, old_size, new_size, align, name)      \
    ({                                                                      \
        void *ptr = MLSL_REALLOC_IMPL(old_ptr, old_size, new_size, align);  \
        MLSL_THROW_IF_NOT(ptr, "MLSL Out of memory, %s", name);             \
        ptr;                                                                \
    })

#define MLSL_CALLOC_WRAPPER(size, align, name)                  \
    ({                                                          \
        void *ptr = MLSL_CALLOC_IMPL(size, align);              \
        MLSL_THROW_IF_NOT(ptr, "MLSL Out of memory, %s", name); \
        ptr;                                                    \
    })

#define MLSL_MALLOC(size, name)                                MLSL_MEMALIGN_WRAPPER(size, CACHELINE_SIZE, name)
#define MLSL_MEMALIGN(size, align, name)                       MLSL_MEMALIGN_WRAPPER(size, align, name)
#define MLSL_CALLOC(size, name)                                MLSL_CALLOC_WRAPPER(size, CACHELINE_SIZE, name)
#define MLSL_REALLOC(old_ptr, old_size, new_size, align, name) MLSL_REALLOC_WRAPPER(old_ptr, old_size, new_size, align, name)
#define MLSL_FREE(ptr)                                         MLSL_FREE_IMPL(ptr)

/* other */

static inline size_t mlsl_pof2(size_t number)
{
    size_t pof2 = 1;

    while (pof2 <= number)
    {
        pof2 <<= 1;
    }
    pof2 >>= 1;

    return pof2;
}

static inline size_t mlsl_aligned_sz(size_t size,
                                     size_t alignment)
{
    return ((size % alignment) == 0) ?
           size : ((size / alignment) + 1) * alignment;
}
