#pragma once

#include "mlsl_types.hpp"
#include "common/env/env.hpp"
#include "common/datatype/datatype.hpp"

#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>

#define GET_TID()    syscall(SYS_gettid)

constexpr const size_t THROW_MESSAGE_LEN = 1024 * 2;

#define __FILENAME__                                                        \
({                                                                          \
        const char *ptr = strrchr(__FILE__, '/');                           \
        if(ptr)                                                             \
        {                                                                   \
            ++ptr;                                                          \
        }                                                                   \
        else                                                                \
        {                                                                   \
            ptr = __FILE__;                                                 \
        }                                                                   \
        ptr;                                                                \
})

#define MLSL_LOG(log_lvl, fmt, ...)                                                    \
  do {                                                                                 \
        if (static_cast<int>(log_lvl) <= env_data.log_level)                           \
        {                                                                              \
            char time_buf[20]; /*2016:07:21 14:47:39*/                                 \
            mlsl_log_get_time(time_buf, 20);                                           \
            switch (log_lvl)                                                           \
            {                                                                          \
                case ERROR:                                                            \
                {                                                                      \
                    fprintf(stderr,"%s: ERROR: (%ld): %s:%u " fmt "\n",                \
                        time_buf, GET_TID(),__FUNCTION__, __LINE__, ##__VA_ARGS__);    \
                    mlsl_log_print_backtrace();                                        \
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


/**
 * Macro to handle critical unrecoverable error. Can be used in destructors
 */
#define MLSL_FATAL(fmt, ...)                                                        \
do                                                                                  \
{                                                                                   \
    fprintf(stderr, "(%ld): %s:%s:%d: FATAL ERROR: " fmt "\n",                      \
            GET_TID(), __FILENAME__, __FUNCTION__, __LINE__, ##__VA_ARGS__);        \
    fflush(stderr);                                                                 \
    mlsl_log_print_backtrace();                                                     \
    std::terminate();                                                               \
} while(0)


/**
 * Helper macro to throw mlsl::mlsl_error exception. Must never be used in destructors
 */
#define MLSL_THROW(fmt, ...)                                                        \
do                                                                                  \
{                                                                                   \
    char msg[THROW_MESSAGE_LEN];                                                    \
    snprintf(msg, THROW_MESSAGE_LEN, "(%ld): %s:%s:%d: EXCEPTION: " fmt "\n",       \
            GET_TID(), __FILENAME__, __FUNCTION__, __LINE__, ##__VA_ARGS__);        \
    mlsl_log_print_backtrace();                                                     \
    throw mlsl::mlsl_error(msg);                                                    \
} while(0)

/**
 * Helper macro to throw mlsl::mlsl_error exception if provided condition is not true.
 * Must never be used in destructors
 */
#define MLSL_THROW_IF_NOT(cond, fmt, ...)                                         \
do                                                                                \
{                                                                                 \
    if (!(cond))                                                                  \
    {                                                                             \
        fprintf(stderr, "(%ld): condition '%s' FAILED\n", GET_TID(), #cond);      \
        MLSL_THROW(fmt, ##__VA_ARGS__);                                           \
    }                                                                             \
} while(0)


#define MLSL_UNUSED(expr) do { (void)sizeof(expr); } while(0)

#ifdef ENABLE_DEBUG

/**
 * Raises failed assertion if provided condition is not true. Works in debug build only
 */
#define MLSL_ASSERT(cond, fmt, ...)                                               \
do                                                                                \
{                                                                                 \
    if (!(cond))                                                                  \
    {                                                                             \
        fprintf(stderr, "(%ld): ASSERT FAILED '%s'\n", GET_TID(), #cond);         \
        assert(0);                                                                \
    }                                                                             \
} while(0)

#else

/**
 * Raises failed assertion if provided condition is not true. Works in debug build only
 */
#define MLSL_ASSERT(cond, fmt, ...) MLSL_UNUSED(cond)

#endif

enum mlsl_log_level
{
    ERROR = 0,
    INFO,
    DEBUG,
    TRACE
};

void mlsl_log_get_time(char* buf,
                       size_t buf_size);
void mlsl_log_print_backtrace(void);
void mlsl_log_print_buffer(const void* buf,
                           size_t cnt,
                           mlsl_datatype_internal_t dtype,
                           const char* prefix);
