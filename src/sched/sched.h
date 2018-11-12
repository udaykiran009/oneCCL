#ifndef SCHED_H
#define SCHED_H

#ifdef __cplusplus
extern "C" {
#endif

#include "atl/atl.h"
#include "coll/coll.h"
#include "common/comm/comm.h"
#include "common/log/log.h"
#include "common/request/request.h"
#include "common/utils/lock.h"
#include "mlsl.h"

#define MLSL_MATCH_ID_MAX_LEN (64)

struct mlsl_sched_queue;
struct mlsl_sched_queue_bin;
struct mlsl_coll_param;
struct mlsl_sched;

enum mlsl_sched_entry_type
{
    mlsl_sched_entry_send        = 0,
    mlsl_sched_entry_recv        = 1,
    mlsl_sched_entry_reduce      = 2,
    mlsl_sched_entry_recv_reduce = 3,
    mlsl_sched_entry_copy        = 4,
    mlsl_sched_entry_compute     = 5,
    mlsl_sched_entry_sync        = 6,
    mlsl_sched_entry_nop         = 7
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

struct mlsl_sched_reduce_local
{
    const void *in_buf;
    size_t in_count;
    void *inout_buf;
    size_t *out_count;
    mlsl_data_type_t dtype;
    mlsl_reduction_t op;
    mlsl_reduction_fn_t reduction_fn;
};
typedef struct mlsl_sched_reduce_local mlsl_sched_reduce_local;

struct mlsl_sched_recv_reduce
{
    void *inout_buf;
    size_t in_count;
    size_t *out_count;
    mlsl_data_type_t dtype;
    mlsl_reduction_t op;
    mlsl_reduction_fn_t reduction_fn;
    size_t src;
    void* comm_buf;
    mlsl_comm *comm;
    atl_req_t req;
};
typedef struct mlsl_sched_recv_reduce mlsl_sched_recv_reduce;

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

struct mlsl_sched_sync
{
    /* Initial value of the counter, used to restore value of the counter */
    int initial_counter;
    /* Current working counter */
    int counter;
    /* Pointer to the root schedule which sync entry will be used to check completion */
    struct mlsl_sched* root_sched;
    /* Index of the current sync entry in the array of entries */
    size_t entry_idx;
    int was_used;
};
typedef struct mlsl_sched_sync mlsl_sched_sync;

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
        mlsl_sched_reduce_local reduce;
        mlsl_sched_recv_reduce recv_reduce;
        mlsl_sched_copy copy;
        mlsl_sched_compute compute;
        mlsl_sched_sync sync;
    } u;
};
typedef struct mlsl_sched_entry mlsl_sched_entry;

enum mlsl_sched_memory_type
{
    mlsl_sched_memory_buffer     = 0,
    mlsl_sched_memory_registered = 1
};
typedef enum mlsl_sched_memory_type mlsl_sched_memory_type;

struct mlsl_sched_memory
{
    mlsl_sched_memory_type type;
    void *ptr;

    struct mlsl_sched_memory *next;
    struct mlsl_sched_memory *prev;
};
typedef struct mlsl_sched_memory mlsl_sched_memory;

struct mlsl_sched_coll_attr
{
    mlsl_prologue_fn_t prologue_fn;
    mlsl_epilogue_fn_t epilogue_fn;
    mlsl_reduction_fn_t reduction_fn;
    size_t priority;
    int synchronous;
    char match_id[MLSL_MATCH_ID_MAX_LEN];
    int to_cache;
};
typedef struct mlsl_sched_coll_attr mlsl_sched_coll_attr;

struct mlsl_sched_coll_param
{
    enum mlsl_coll_type ctype;
    void *buf;
    const void *send_buf;
    void *recv_buf;
    size_t count;
    size_t send_count;
    size_t *recv_counts;
    mlsl_data_type_t dtype;
    mlsl_reduction_t reduction;
    size_t root;
    mlsl_comm *comm;
};
typedef struct mlsl_sched_coll_param mlsl_sched_coll_param;

struct mlsl_sched
{
    int first_progress;
    struct mlsl_sched_queue_bin *bin;
    mlsl_sched_coll_param coll_param;
    mlsl_sched_coll_attr coll_attr;
    size_t size;               /* capacity (in entries) of the entries array */
    size_t idx;                /* index into entries array of first yet-outstanding entry */ /* index to start */
    size_t num_entries;        /* number of populated entries, num_entries <= size */
    mlsl_sched_id_t sched_id;   /* sequence number of the schedule in the communicator */
    struct mlsl_request *req;
    mlsl_sched_entry *entries;
    mlsl_sched_memory *persistent_memory;

    struct mlsl_sched **partial_scheds;
    size_t partial_sched_count;

    struct mlsl_sched *next;   /* linked-list next pointer */
    struct mlsl_sched *prev;   /* linked-list prev pointer */
};
typedef struct mlsl_sched mlsl_sched;

mlsl_status_t mlsl_sched_create(mlsl_sched **sched);
mlsl_status_t mlsl_sched_commit(mlsl_sched *sched);
mlsl_status_t mlsl_sched_start(mlsl_sched *sched, struct mlsl_request **req);
mlsl_status_t mlsl_sched_free(mlsl_sched *sched);

mlsl_status_t mlsl_sched_add_send(mlsl_sched *sched, const void *buf, size_t count,
                                           mlsl_data_type_t dtype, size_t dest);
mlsl_status_t mlsl_sched_add_recv(mlsl_sched *sched, void *buf, size_t count,
                                           mlsl_data_type_t data_type, size_t src);
mlsl_status_t mlsl_sched_add_reduce(mlsl_sched *sched, const void *in_buf, size_t in_count,
                                             void *inout_buf, size_t *out_count,
                                             mlsl_data_type_t dtype, mlsl_reduction_t reduction);
/**
 * Combination of recv and reduce operations.
 * @param sched Pointer to schedule for adding entries
 * @param inout_buf Buffer with local data, will hold result of reduction.
 * @param count Number of elements in inout_buf for reduce operation and number of elements in comm_buf for receive operation
 * @param out_count Pointer to buffer to hold number of elements in inout_buf after reduce operation. Can be NULL
 * @param dtype Data type of inout_buf and comm_buf
 * @param op Reduction operation, see @ref mlsl_reduction_t
 * @param src Remote rank ID for receiving data
 * @param comm_buf Optional buffer for communication. Can be a @B NULL, in that case MLSL will allocate temporal buffer
 */
mlsl_status_t mlsl_sched_add_recv_reduce(mlsl_sched *sched, void *inout_buf, size_t count,
                                         size_t *out_count, mlsl_data_type_t dtype,
                                         mlsl_reduction_t op, size_t src,
                                         void* comm_buf);

mlsl_status_t mlsl_sched_add_copy(mlsl_sched *sched, const void *in_buf,
                                           void *out_buf, size_t count, mlsl_data_type_t dtype);
mlsl_status_t mlsl_sched_add_barrier(mlsl_sched *sched);
mlsl_status_t mlsl_sched_add_sync(mlsl_sched *sched, mlsl_sched_entry **sync_entry);
mlsl_status_t mlsl_sched_sync_schedules(mlsl_sched **scheds, size_t count);

mlsl_status_t mlsl_sched_add_compute_1i1o(mlsl_sched *sched, mlsl_sched_compute_1i1o_fn_t cb_p,
                                          const void *in_buf, size_t in_count,
                                          void *out_buf, size_t *out_count,
                                          mlsl_data_type_t dtype);

mlsl_status_t mlsl_sched_set_prologue(mlsl_sched *sched, mlsl_prologue_fn_t fn);
mlsl_status_t mlsl_sched_set_epilogue(mlsl_sched *sched, mlsl_epilogue_fn_t fn);
mlsl_status_t mlsl_sched_set_reduction(mlsl_sched *sched, mlsl_reduction_fn_t fn);

mlsl_status_t mlsl_sched_bcast(void *buf, size_t count, mlsl_data_type_t dtype,
                               size_t root, mlsl_comm* comm, mlsl_sched **sched);
mlsl_status_t mlsl_sched_reduce(const void *send_buf, void *recv_buf, size_t count, mlsl_data_type_t dtype,
                                mlsl_reduction_t reduction, size_t root, mlsl_comm* comm, mlsl_sched **sched);
mlsl_status_t mlsl_sched_allreduce(const void *send_buf, void *recv_buf, size_t count, mlsl_data_type_t dtype,
                                   mlsl_reduction_t reduction, mlsl_comm* comm, mlsl_sched **sched);
mlsl_status_t mlsl_sched_allgatherv(const void *send_buf, size_t send_count, void *recv_buf, size_t *recv_counts,
                                    mlsl_data_type_t dtype, mlsl_comm* comm, mlsl_sched **sched);
mlsl_status_t mlsl_sched_barrier(mlsl_comm* comm, mlsl_sched **sched);

mlsl_status_t mlsl_sched_progress(struct mlsl_sched_queue_bin *bin, size_t sched_count, size_t *processed_sched_count);
mlsl_status_t mlsl_sched_clone(mlsl_sched *orig, mlsl_sched **clone);
mlsl_status_t mlsl_sched_adjust(mlsl_sched *sched, size_t partition_idx, size_t partition_count);
mlsl_status_t mlsl_sched_adjust_entries(mlsl_sched *sched, size_t partition_idx, size_t partition_count);
mlsl_status_t mlsl_sched_adjust_tag(mlsl_sched *sched);
mlsl_status_t mlsl_sched_dump(mlsl_sched *sched, const char *name);
mlsl_status_t mlsl_sched_reset(mlsl_sched *sched);

mlsl_status_t mlsl_sched_add_persistent_memory(mlsl_sched *sched, mlsl_sched_memory_type type, void *ptr);
mlsl_status_t mlsl_sched_free_persistent_memory(mlsl_sched *sched);

mlsl_status_t mlsl_sched_set_coll_attr(mlsl_sched *sched, const struct mlsl_coll_attr *attr);

mlsl_priority_mode mlsl_sched_get_priority(mlsl_sched *sched);

#ifdef __cplusplus
}
#endif

#endif /* SCHED_H */
