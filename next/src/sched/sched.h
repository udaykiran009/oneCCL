#ifndef SCHED_H
#define SCHED_H

#include "atl.h"
#include "comm.h"
#include "lock.h"
#include "log.h"
#include "mlsl.h"
#include "mlsl_sched.h"
#include "request.h"

#define MAX_SCHED_BINS (1024)

struct mlsl_sched_queue;
struct mlsl_sched_queue_bin;

enum mlsl_sched_type
{
    mlsl_sched_persistent     = 0,
    mlsl_sched_non_persistent = 1
};
typedef enum mlsl_sched_type mlsl_sched_type;

enum mlsl_sched_entry_type
{
    mlsl_sched_entry_send    = 0,
    mlsl_sched_entry_recv    = 1,
    mlsl_sched_entry_reduce  = 2,
    mlsl_sched_entry_copy    = 3,
    mlsl_sched_entry_compute = 4,
    mlsl_sched_entry_nop     = 5
};
typedef enum mlsl_sched_entry_type mlsl_sched_entry_type;

struct mlsl_sched_send
{
    const void *buf;
    size_t count;
    mlsl_data_type_t dtype;
    size_t dest;
    mlsl_comm *comm;
    // TODO_ATL: replace by handle instead of explicit structure
    atl_req_t req;
};
typedef struct mlsl_sched_send mlsl_sched_send;

struct mlsl_sched_recv
{
    void *buf;
    size_t count;
    mlsl_data_type_t dtype;
    size_t src;
    mlsl_comm *comm;
    atl_req_t req;
};
typedef struct mlsl_sched_recv mlsl_sched_recv;

struct mlsl_sched_reduce
{
    const void *in_buf;
    size_t in_count;
    void *inout_buf;
    size_t *out_count;
    mlsl_data_type_t dtype;
    mlsl_reduction_t op;
};
typedef struct mlsl_sched_reduce mlsl_sched_reduce;

struct mlsl_sched_copy
{
    const void *in_buf;
    void *out_buf;
    size_t count;
    mlsl_data_type_t dtype;
};
typedef struct mlsl_sched_copy mlsl_sched_copy;

enum mlsl_sched_compute_type
{
    mlsl_sched_compute_1i1o = 0,
};
typedef enum mlsl_sched_compute_type mlsl_sched_compute_type;

typedef mlsl_status_t(*mlsl_sched_compute_1i1o_fn_t) (const void*, size_t, void*, size_t*, mlsl_data_type_t);

struct mlsl_sched_compute
{
    enum mlsl_sched_compute_type type;
    int is_parallelizable;
    union
    {
        mlsl_sched_compute_1i1o_fn_t fn_1i1o;
    } u;
    const void *in_buf;
    size_t in_count;
    void *out_buf;
    size_t *out_count;
    mlsl_data_type_t dtype;
};
typedef struct mlsl_sched_compute mlsl_sched_compute;

enum mlsl_sched_entry_status
{
    mlsl_sched_entry_status_not_started = 0,
    mlsl_sched_entry_status_started     = 1,
    mlsl_sched_entry_status_complete    = 2,
    mlsl_sched_entry_status_failed      = 3,
    mlsl_sched_entry_status_invalid     = 4
};
typedef enum mlsl_sched_entry_status mlsl_sched_entry_status;

struct mlsl_sched_entry
{
    enum mlsl_sched_entry_type type;
    enum mlsl_sched_entry_status status;
    int is_barrier;
    union
    {
        mlsl_sched_send send;
        mlsl_sched_recv recv;
        mlsl_sched_reduce reduce;
        mlsl_sched_copy copy;
        mlsl_sched_compute compute;
    } u;
};
typedef struct mlsl_sched_entry mlsl_sched_entry;

struct mlsl_sched
{
    struct mlsl_sched_queue_bin *bin;

    mlsl_sched_type type;
    size_t size;               /* capacity (in entries) of the entries array */
    size_t idx;                /* index into entries array of first yet-outstanding entry */ /* index to start */
    size_t num_entries;        /* number of populated entries, num_entries <= size */
    int tag;
    struct mlsl_request *req;
    mlsl_sched_entry *entries;

    struct mlsl_sched *next;   /* linked-list next pointer */
    struct mlsl_sched *prev;   /* linked-list next pointer */
};
typedef struct mlsl_sched mlsl_sched;

mlsl_status_t mlsl_sched_add_compute_1i1o(mlsl_sched *sched, mlsl_sched_compute_1i1o_fn_t cb_p,
                                          const void *in_buf, size_t in_count,
                                          void *out_buf, size_t *out_count,
                                          mlsl_data_type_t dtype);
mlsl_status_t mlsl_sched_progress(struct mlsl_sched_queue_bin *bin, size_t sched_count, size_t *processed_sched_count);
mlsl_status_t mlsl_sched_next_tag(mlsl_comm *comm, int *tag);
mlsl_status_t mlsl_sched_clone(mlsl_sched *orig, mlsl_sched **cloned);
mlsl_status_t mlsl_sched_adjust(mlsl_sched *sched, size_t partition_idx, size_t partition_count);
mlsl_status_t mlsl_sched_dump(mlsl_sched *sched);
mlsl_status_t mlsl_sched_reset(mlsl_sched *sched);
mlsl_status_t mlsl_sched_commit_with_type(mlsl_sched *sched, mlsl_sched_type type);

struct mlsl_sched_queue_bin
{
    struct mlsl_sched_queue *queue;
    size_t priority;
    mlsl_sched *elems;
    size_t elem_count;

    atl_comm_t *comm_ctx;

    // TODO:
    void *comp_ctx;
};
typedef struct mlsl_sched_queue_bin mlsl_sched_queue_bin;

struct mlsl_sched_queue
{
    mlsl_fastlock_t lock;
    size_t used_bins;
    size_t max_bins;
    size_t max_priority;
    mlsl_sched_queue_bin bins[MAX_SCHED_BINS];
};
typedef struct mlsl_sched_queue mlsl_sched_queue;

mlsl_status_t mlsl_sched_queue_create(size_t max_bins, atl_comm_t **comm_ctxs, mlsl_sched_queue **queue);
mlsl_status_t mlsl_sched_queue_free(mlsl_sched_queue *queue);
mlsl_status_t mlsl_sched_queue_add(mlsl_sched_queue *queue, mlsl_sched *sched, size_t priority);
mlsl_status_t mlsl_sched_queue_remove(mlsl_sched_queue *queue, mlsl_sched_queue_bin *bin, mlsl_sched *sched);
mlsl_status_t mlsl_sched_queue_peek(mlsl_sched_queue *queue, mlsl_sched_queue_bin **bin, size_t *count);

#endif /* SCHED_H */
