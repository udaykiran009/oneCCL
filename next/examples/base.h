#ifndef BASE_H
#define BASE_H

#include "assert.h"
#include <errno.h>
#include <inttypes.h>
#include "math.h"
#include <stdint.h>
#include "stdio.h"
#include <stdlib.h>
#include "string.h"
#include <sys/time.h>
#include <unistd.h>

#include "mlsl.h"

#define COUNT 1048576
#define ITERS 128
#define ROOT  0
#define MLSL_CALL(expr)                             \
  do {                                              \
        mlsl_status_t status = mlsl_status_success; \
        status = expr;                              \
        assert(status == mlsl_status_success);      \
  } while (0)

mlsl_coll_attr_t coll_attr;
mlsl_request_t request;
size_t proc_idx, proc_count;
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

#endif /* BASE_H */
