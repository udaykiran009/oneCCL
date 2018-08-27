#ifndef UTILS_H
#define UTILS_H

#if defined(__INTEL_COMPILER) || defined(__ICC)
#include <mm_malloc.h>
#endif
#include <stdlib.h>

/* common */

#define MLSL_CALL(expr)                              \
  do {                                               \
        status = expr;                               \
        MLSL_ASSERT(status == mlsl_status_success); \
  } while (0)

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))

#define unlikely(x_) __builtin_expect(!!(x_), 0)
#define likely(x_)   __builtin_expect(!!(x_), 1)

#define CACHELINE_SIZE 64
#define ONE_MB         1048576
#define TWO_MB         2097152

/* doubly linked list */

#define MLSL_DLIST_FOREACH(head,el) MLSL_DLIST_FOREACH_NP(head, el, next, prev)
#define MLSL_DLIST_FOREACH_NP(head, el, _next, _prev) \
    for (el = head; el; el = el->_next)

/* this version is safe for deleting the elements during iteration */
#define MLSL_DLIST_FOREACH_SAFE(head, el, tmp) MLSL_DLIST_FOREACH_SAFE_NP(head, el, tmp, next, prev)
#define MLSL_DLIST_FOREACH_SAFE_NP(head, el, tmp, _next, _prev) \
  for ((el)=(head); (el) && (tmp = (el)->_next, 1); (el) = tmp)

#define MLSL_DLIST_PREPEND(head, add) MLSL_DLIST_PREPEND_NP(head, add, next, prev)
#define MLSL_DLIST_PREPEND_NP(head, add, _next, _prev) \
  do {                                                 \
       (add)->_next = head;                            \
       if (head) {                                     \
         (add)->_prev = (head)->_prev;                 \
         (head)->_prev = (add);                        \
       } else {                                        \
         (add)->_prev = (add);                         \
       }                                               \
       (head) = (add);                                 \
  } while (0)

#define MLSL_DLIST_APPEND(head, add) MLSL_DLIST_APPEND_NP(head, add, next, prev)
#define MLSL_DLIST_APPEND_NP(head,add,_next,_prev) \
  do {                                             \
      if (head) {                                  \
          (add)->_prev = (head)->_prev;            \
          (head)->_prev->_next = (add);            \
          (head)->_prev = (add);                   \
          (add)->_next = NULL;                     \
      } else {                                     \
          (head)=(add);                            \
          (head)->_prev = (head);                  \
          (head)->_next = NULL;                    \
      }                                            \
  } while (0)

#define MLSL_DLIST_DELETE(head, del) MLSL_DLIST_DELETE_NP(head,del,next,prev)
#define MLSL_DLIST_DELETE_NP(head, del, _next, _prev) \
do {                                                  \
    if ((del)->_prev == (del)) {                      \
        (head) = NULL;                                \
    } else if ((del)==(head)) {                       \
        (del)->_next->_prev = (del)->_prev;           \
        (head) = (del)->_next;                        \
    } else {                                          \
        (del)->_prev->_next = (del)->_next;           \
        if ((del)->_next) {                           \
            (del)->_next->_prev = (del)->_prev;       \
        } else {                                      \
            (head)->_prev = (del)->_prev;             \
        }                                             \
    }                                                 \
} while (0)

/* malloc/realloc/free */

#if defined(__INTEL_COMPILER) || defined(__ICC)
#define MLSL_MEMALIGN_IMPL(size, align) _mm_malloc(size, align)
#define MLSL_REALLOC_IMPL(old_ptr, old_size, new_size, align)    \
      ({                                                         \
          void* new_ptr = NULL;                                  \
          if (!old_ptr) new_ptr = _mm_malloc(new_size, align);   \
          else if (!old_size) _mm_free(old_ptr);                 \
          else                                                   \
          {                                                      \
              new_ptr = _mm_malloc(new_size, align);             \
              memcpy(new_ptr, old_ptr, MIN(old_size, new_size)); \
              _mm_free(old_ptr);                                 \
          }                                                      \
          new_ptr;                                               \
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
      void* ptr;                                                                        \
      int pm_ret __attribute__((unused)) = posix_memalign((void**)(&ptr), align, size); \
      ptr;                                                                              \
    })
#define MLSL_REALLOC_IMPL(old_ptr, old_size, new_size, align) realloc(old_ptr, new_size)
#define MLSL_CALLOC_IMPL(size, align) calloc(size, 1)
#define MLSL_FREE_IMPL(ptr) free(ptr)
#else
# error "this compiler is not supported" 
#endif

#define MLSL_MEMALIGN_WRAPPER(size, align, name)   \
    ({                                             \
      void *ptr = MLSL_MEMALIGN_IMPL(size, align); \
      MLSL_ASSERT_FMT(ptr, name);                  \
      ptr;                                         \
    })

#define MLSL_REALLOC_WRAPPER(old_ptr, old_size, new_size, align, name)   \
    ({                                                                   \
      void *ptr = MLSL_REALLOC_IMPL(old_ptr, old_size, new_size, align); \
      MLSL_ASSERT_FMT(ptr, name);                                        \
      ptr;                                                               \
    })

#define MLSL_CALLOC_WRAPPER(size, align, name)   \
    ({                                           \
      void *ptr = MLSL_CALLOC_IMPL(size, align); \
      MLSL_ASSERT_FMT(ptr, name);                \
      ptr;                                       \
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
        pof2 <<= 1;
    pof2 >>= 1;

    return pof2;
}

#endif /* UTILS_H */
