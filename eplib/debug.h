#ifndef _DEBUG_H_
#define _DEBUG_H_

/* Usage:
   Default case:
   ASSERT: assert in non-critical path test.
   When ENABLE_DEBUG is defined
   DEBUG_SUCCESS: quick check if control reached.
   DEBUG_PRINT: print some meaningful output.
   DEBUG_ASSERT: assert in critical path test.
*/

#include <errno.h>
#include <sys/syscall.h>

#define gettid()     syscall(SYS_gettid)
#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#define ASSERT(cond)                                           \
  do                                                           \
  {                                                            \
      if (!(cond))                                             \
      {                                                        \
          fprintf(stdout, "@: %s:%d:%s() " #cond " failed.\n", \
                  __FILENAME__, __LINE__, __FUNCTION__);       \
          fflush(stdout);                                      \
          MPI_Finalize();                                      \
          exit(1);                                             \
      }                                                        \
  } while(0)

#define ASSERT_FMT(cond, fmt, ...)                            \
  do                                                          \
  {                                                           \
      if (!(cond))                                            \
      {                                                       \
          fprintf(stdout, "@: %s:%d:%s() " #fmt " failed.\n", \
                  __FILENAME__, __LINE__,                     \
                  __FUNCTION__, ##__VA_ARGS__);               \
          fflush(stdout);                                     \
          MPI_Finalize();                                     \
          exit(1);                                            \
      }                                                       \
  } while(0)

#define PRINT(s, ...)                                   \
    do                                                  \
    {                                                   \
        char hoststr[32];                               \
        gethostname(hoststr,sizeof hoststr);            \
        fprintf(stdout, "%s: @ %s:%d:%s() " s, hoststr, \
                __FILENAME__, __LINE__,                 \
                __func__, ##__VA_ARGS__);               \
        fflush(stdout);                                 \
    } while (0)

#define ERROR(s, ...)                                   \
    do                                                  \
    {                                                   \
        char hoststr[32];                               \
        gethostname(hoststr,sizeof hoststr);            \
        fprintf(stdout, "%s: @ %s:%d:%s() " s, hoststr, \
                __FILENAME__, __LINE__,                 \
                __func__, ##__VA_ARGS__);               \
        fflush(stdout);                                 \
        MPI_Finalize();                                 \
        exit(-1);                                       \
    } while (0)

#define NOW_HERE()                        \
    do                                    \
    {                                     \
        fprintf(stdout, "@ %s:%d:%s()\n", \
                __FILENAME__, __LINE__,   \
                __func__);                \
        fflush(stdout);                   \
    } while (0)

#ifdef ENABLE_DEBUG

#include <stdarg.h>
#include <stdio.h>

#define DEBUG_ASSERT(cond) ASSERT(cond)

#define DEBUG_PRINT(s, ...)                           \
    do                                                \
    {                                                 \
        char hoststr[32];                             \
        gethostname(hoststr,sizeof hoststr);          \
        fprintf(stdout, "(%zu): %s: @ %s:%d:%s() " s, \
                gettid(), hoststr,                    \
                __FILENAME__, __LINE__,               \
                __func__, ##__VA_ARGS__);             \
        fflush(stdout);                               \
    } while (0)

#define DEBUG_TEST(cond, ...)                                 \
    do                                                        \
    {                                                         \
      if(!(cond))                                             \
      {                                                       \
          fprintf(stdout, "@ %s:%d:%s() " #cond " failed.\n", \
                  __FILENAME__, __LINE__, __func__);          \
          fprintf(##__VA_ARGS__);                             \
          fflush(stdout);                                     \
      }                                                       \
    } while (0)

#else /* !ENABLE_DEBUG */

#define DEBUG_ASSERT(cond)
#define DEBUG_PRINT(s, ...)
#define DEBUG_TEST(cond, ...)

#endif /* ENABLE_DEBUG */

#endif /* _DEBUG_H_ */
