#ifndef DEF_INCLUDED
#define DEF_INCLUDED

#include <string.h>

//TODO: change exit to something more useful
#define SET_STR(dst, size, ...) \
    do { \
        if (snprintf(dst, size, __VA_ARGS__) > size) { \
            printf("Line so big (must be low %d)\n", size); \
            printf(__VA_ARGS__); \
            exit(1); \
        } \
    } while (0)

#define CHECK_FGETS(expr, str) \
    do { \
        char* res = expr; \
        if (!res || res != str) { \
            printf("fgets error\n"); \
            exit(EXIT_FAILURE); \
        } \
    } while (0)

#define DO_RW_OP(op, fd, buf, size, memory_mutex) \
    do { \
        { \
            std::lock_guard<std::mutex> lock(memory_mutex); \
            ssize_t res = 0; \
            size_t shift = 0; \
            while (shift != size) { \
                res = op(fd, (char*)buf + shift, size - shift); \
                if (res == -1) { \
                    if (errno != EINTR) { \
                        printf("read/write error: %s\n", strerror(errno)); \
                        exit(EXIT_FAILURE); \
                    } \
                } \
                else { \
                    shift += res; \
                } \
            } \
        } \
    } while (0)


#define DO_RW_OP_1(op, fd, buf, size, res) \
    do { \
        size_t shift = 0; \
        res = 0; \
        do { \
            res = op(fd, (char*)buf + shift, size - shift); \
            if (res == -1) { \
                if (errno != EINTR) { \
                    printf("read/write error: %s\n", strerror(errno)); \
                    exit(EXIT_FAILURE); \
                } \
            }\
            else { \
                shift += res; \
            } \
        } while (shift != size && res != 0); \
    } while (0)

#define BARRIER_NUM_MAX         1024
#define MAX_KVS_KEY_LENGTH      130
#define MAX_KVS_VAL_LENGTH      130
#define MAX_KVS_NAME_LENGTH     130
#define MAX_KVS_NAME_KEY_LENGTH (MAX_KVS_NAME_LENGTH + MAX_KVS_KEY_LENGTH + 1)
#define RUN_TEMPLATE_SIZE       1024
#define INT_STR_SIZE            8
#define REQUEST_POSTFIX_SIZE    1024
#define RUN_REQUEST_SIZE        2048

#define CCL_WORLD_SIZE_ENV "CCL_WORLD_SIZE"

#define KVS_NAME_TEMPLATE_I         "%s_%zu"
#define KVS_NAME_TEMPLATE_S         "%s%s"
#define KVS_NAME_KEY_TEMPLATE       "%s-%s"
#define GREP_TEMPLATE               "| grep \"%s\""
#define GREP_COUNT_TEMPLATE         "| grep -c \"%s\""
#define CONCAT_TWO_COMMAND_TEMPLATE "%s %s"
#define SIZE_T_TEMPLATE             "%zu"

#define KVS_NAME    "CCL_POD_ADDR"
#define KVS_BARRIER "CCL_BARRIER"
#define KVS_BARRIER_FULL "CCL_BARRIER_FULL"

#define KVS_IDX               "IDX"
#define KVS_UP                "CCL_UP"
#define KVS_POD_REQUEST       "CCL_POD_REQUEST"
#define KVS_POD_NUM           "CCL_POD_NUM"
#define KVS_NEW_POD           "CCL_NEW_POD"
#define KVS_DEAD_POD          "CCL_DEAD_POD"
#define KVS_APPROVED_NEW_POD  "CCL_APPROVED_NEW_POD"
#define KVS_APPROVED_DEAD_POD "CCL_APPROVED_DEAD_POD"
#define KVS_ACCEPT            "CCL_ACCEPT"

#define CCL_IP_LEN 128

#define CHECKER_IP         "hostname -I"
#define READ_ONLY          "r"
#define NULL_CHAR          '\0'
#define MAX_UP_IDX         2048
#define INITIAL_UPDATE_IDX "0"
#define INITIAL_RANK_NUM   "0"
#define MAX_CLEAN_CHECKS   3

#define STR_COPY(dst, src, len) { memcpy((dst), (src), (len-1)); dst[len - 1] = '\0'; }

extern char my_hostname[MAX_KVS_VAL_LENGTH];

#endif
