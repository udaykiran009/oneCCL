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

#include "mlsl.h"

#define COUNT 1048576
#define ITERS 100
#define ROOT  0
#define MLSL_CALL(expr)                             \
  do {                                              \
        mlsl_status_t status = mlsl_status_success; \
        status = expr;                              \
        assert(status == mlsl_status_success);      \
        (void)status;                               \
  } while (0)

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

mlsl_coll_attr_t coll_attr;
mlsl_request_t request;
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
    MLSL_CALL(mlsl_init());

    MLSL_CALL(mlsl_get_comm_rank(NULL, &rank));
    MLSL_CALL(mlsl_get_comm_size(NULL, &size));

    coll_attr.prologue_fn = NULL;
    coll_attr.epilogue_fn = NULL;
    coll_attr.reduction_fn = NULL;
    coll_attr.priority = 0;
    coll_attr.synchronous = 0;
    coll_attr.match_id = NULL;
    coll_attr.to_cache = 0;
}

void test_finalize()
{
    MLSL_CALL(mlsl_finalize());
}

#endif /* BASE_H */
