#ifndef BASE_H
#define BASE_H

#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include "iccl.h"

#define COUNT     1048576
#define ITERS     100
#define COLL_ROOT 0

void test_finalize();

#define ASSERT(cond, fmt, ...)                            \
  do                                                      \
  {                                                       \
      if (!(cond))                                        \
      {                                                   \
          fprintf(stderr, "ASSERT '%s' FAILED " fmt "\n", \
              #cond, ##__VA_ARGS__);                      \
          test_finalize();                                \
          exit(1);                                        \
      }                                                   \
  } while (0)

#define ICCL_CALL(expr)                                      \
  do {                                                       \
        iccl_status_t status = iccl_status_success;          \
        status = expr;                                       \
        ASSERT(status == iccl_status_success, "ICCL error"); \
        (void)status;                                        \
  } while (0)

iccl_coll_attr_t coll_attr;
iccl_request_t request;
size_t rank, size;
double t1, t2, t;
size_t idx, iter_idx;

double when(void)
{
    struct timeval tv;
    static struct timeval tv_base;
    static int is_first = 1;

    if (gettimeofday(&tv, NULL)) {
        perror("gettimeofday");
        return 0;
    }

    if (is_first) {
        tv_base = tv;
        is_first = 0;
    }
    return (double)(tv.tv_sec - tv_base.tv_sec) * 1.0e6 + (double)(tv.tv_usec - tv_base.tv_usec);
}

void test_init()
{
    ICCL_CALL(iccl_init());

    ICCL_CALL(iccl_get_comm_rank(NULL, &rank));
    ICCL_CALL(iccl_get_comm_size(NULL, &size));

    coll_attr.prologue_fn = NULL;
    coll_attr.epilogue_fn = NULL;
    coll_attr.reduction_fn = NULL;
    coll_attr.priority = 0;
    coll_attr.synchronous = 0;
    coll_attr.match_id = "tensor_name";
    coll_attr.to_cache = 0;
}

void test_finalize()
{
    ICCL_CALL(iccl_finalize());
}

#endif /* BASE_H */
