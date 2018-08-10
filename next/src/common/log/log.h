#ifndef LOG_H
#define LOG_H

#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>

#include "env.h"
#include "mlsl.h"

#define GET_TID()    syscall(SYS_gettid)
#define IS_SPACE(c)  ((c==0x20 || c==0x09 || c==0x0a || c==0x0b || c==0x0c || c==0x0d) ? 8 : 0)
#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#define MLSL_LOG(log_lvl, fmt, ...)                                                    \
  do {                                                                                 \
        if (log_lvl <= env_data.log_level)                                             \
        {                                                                              \
            char time_buf[20]; /*2016:07:21 14:47:39*/                                 \
            mlsl_log_get_time(time_buf, 20);                                           \
            switch (log_lvl)                                                           \
            {                                                                          \
                case ERROR:                                                            \
                {                                                                      \
                    printf("%s: ERROR: (%ld): %s:%u " fmt "\n", time_buf, GET_TID(),   \
                            __FUNCTION__, __LINE__, ##__VA_ARGS__);                    \
                    mlsl_log_print_backtrace();                                        \
                    mlsl_finalize();                                                   \
                    _exit(1);                                                          \
                    break;                                                             \
                }                                                                      \
                case INFO:                                                             \
                {                                                                      \
                    printf("(%ld): %s:%u " fmt "\n", GET_TID(),                        \
                            __FUNCTION__, __LINE__, ##__VA_ARGS__);                    \
                    break;                                                             \
                }                                                                      \
                case DEBUG:                                                            \
                case TRACE:                                                            \
                {                                                                      \
                    printf("%s: (%ld): %s:%u " fmt "\n", time_buf, GET_TID(),          \
                            __FUNCTION__, __LINE__, ##__VA_ARGS__);                    \
                    break;                                                             \
                }                                                                      \
                default:                                                               \
                {                                                                      \
                    assert(0);                                                         \
                }                                                                      \
            }                                                                          \
            fflush(stdout);                                                            \
        }                                                                              \
  } while (0)

#define MLSL_ASSERTP_FMT(cond, fmt, ...)                                                  \
  do                                                                                      \
  {                                                                                       \
      if (!(cond))                                                                        \
      {                                                                                   \
          fprintf(stderr, "(%ld): %s:%s:%d: ASSERT '%s' FAILED: " fmt "\n",               \
                  GET_TID(), __FILENAME__, __FUNCTION__, __LINE__, #cond, ##__VA_ARGS__); \
          fflush(stderr);                                                                 \
          mlsl_finalize();                                                                \
          _exit(1);                                                                       \
      }                                                                                   \
  } while(0)
#define MLSL_ASSERTP(cond) MLSL_ASSERTP_FMT(cond, "")


#ifdef ENABLE_DEBUG
#define MLSL_ASSERT_FMT(cond, fmt, ...) MLSL_ASSERTP_FMT(cond, fmt, ##__VA_ARGS__);
#define MLSL_ASSERT(cond) MLSL_ASSERTP(cond);
#else
#define MLSL_ASSERT_FMT(cond, fmt, ...)
#define MLSL_ASSERT(cond)
#endif

enum mlsl_log_level
{
    ERROR = 0,
    INFO,
    DEBUG,
    TRACE
};
typedef enum mlsl_log_level mlsl_log_level;

void mlsl_log_get_time(char* buf, size_t buf_size);
void mlsl_log_print_backtrace(void);

#endif /* LOG_H */
