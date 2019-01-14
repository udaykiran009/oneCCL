#pragma once

#include "atl/atl.h"
#include "coll/coll.hpp"
#include "common/comm/comm.hpp"
#include "common/datatype/datatype.hpp"
#include "common/log/log.hpp"
#include "common/request/request.hpp"
#include "common/utils/lock.hpp"
#include "mlsl.h"

#define MLSL_POSTPONED_COUNT  ((size_t)(0xFFFFFFFFFFFFFFFF))
#define MLSL_POSTPONED_ADDR   ((void*)(0xFFFFFFFFFFFFFFFF))

struct mlsl_sched_queue;
struct mlsl_sched_queue_bin;
struct mlsl_coll_param;
struct mlsl_sched;

namespace out_of_order
{
class ooo_match;
}

enum mlsl_sched_entry_type
{
    mlsl_sched_entry_send                    = 0,
    mlsl_sched_entry_recv                    = 1,
    mlsl_sched_entry_reduce                  = 2,
    mlsl_sched_entry_recv_reduce             = 3,
    mlsl_sched_entry_copy                    = 4,
    mlsl_sched_entry_sync                    = 5,
    mlsl_sched_entry_collective              = 6,
    mlsl_sched_entry_prologue                = 7,
    mlsl_sched_entry_epilogue                = 8,
    mlsl_sched_entry_update_postponed_fields = 9,
    mlsl_sched_entry_tensor_comm             = 10,
    mlsl_sched_entry_nop                     = 11
};

struct mlsl_sched_send
{
    const void *buf;
    size_t count;
    mlsl_datatype_internal_t dtype;
    size_t dest;
    mlsl_comm *comm;
    // TODO_ATL: replace by handle instead of explicit structure
    atl_req_t req;
};

struct mlsl_sched_recv
{
    void *buf;
    size_t count;
    mlsl_datatype_internal_t dtype;
    size_t src;
    mlsl_comm *comm;
    atl_req_t req;
};

struct mlsl_sched_reduce_local
{
    const void *in_buf;
    size_t in_count;
    void *inout_buf;
    size_t *out_count;
    mlsl_datatype_internal_t dtype;
    mlsl_reduction_t op;
    mlsl_reduction_fn_t reduction_fn;
};

struct mlsl_sched_recv_reduce
{
    void *inout_buf;
    size_t in_count;
    size_t *out_count;
    mlsl_datatype_internal_t dtype;
    mlsl_reduction_t op;
    mlsl_reduction_fn_t reduction_fn;
    size_t src;
    void* comm_buf;
    mlsl_comm *comm;
    atl_req_t req;
};

struct mlsl_sched_copy
{
    const void *in_buf;
    void *out_buf;
    size_t count;
    mlsl_datatype_internal_t dtype;
};

struct mlsl_sched_sync
{
    /* Initial value of the counter, used to restore value of the counter */
    int initial_counter;
    /* Current working counter */
    int counter;
    /* Pointer to the root schedule which sync entry will be used to check completion */
    mlsl_sched* root_sched;
    /* Index of the current sync entry in the array of entries */
    size_t entry_idx;
};

struct mlsl_sched_collective
{
    mlsl_request* req;
    mlsl_coll_type ctype;
    const void *send_buf;
    void *recv_buf;
    size_t count;
    mlsl_datatype_internal_t dtype;
    mlsl_reduction_t op;
    mlsl_comm *comm;
    /* TODO: extend for other collectives */
};

struct mlsl_sched_prologue
{
    mlsl_prologue_fn_t fn;
    const void *in_buf;
    size_t in_count;
    mlsl_datatype_internal_t in_dtype;
    void** out_buf;
    size_t* out_count;
    mlsl_datatype_internal* out_dtype;
};

struct mlsl_sched_epilogue
{
    mlsl_epilogue_fn_t fn;
    const void *in_buf;
    size_t in_count;
    mlsl_datatype_internal_t in_dtype;
    void* out_buf;
    size_t* out_count;
    size_t expected_out_count;
    mlsl_datatype_internal_t out_dtype;
};

struct mlsl_sched_update_postponed_fields
{
    size_t part_idx;
    size_t part_count;
};

struct mlsl_sched_tensor_comm
{
    out_of_order::ooo_match* ooo_handler;
    const char* tensor_name;
};

enum mlsl_sched_entry_status
{
    mlsl_sched_entry_status_not_started = 0,
    mlsl_sched_entry_status_started     = 1,
    mlsl_sched_entry_status_complete    = 2,
    mlsl_sched_entry_status_failed      = 3,
    mlsl_sched_entry_status_invalid     = 4
};

struct mlsl_sched_entry
{
    mlsl_sched_entry_type type;
    mlsl_sched_entry_status status;
    int is_barrier;
    union
    {
        mlsl_sched_send send;
        mlsl_sched_recv recv;
        mlsl_sched_reduce_local reduce; // primitive operation
        mlsl_sched_recv_reduce recv_reduce;
        mlsl_sched_copy copy;
        mlsl_sched_sync sync;
        mlsl_sched_collective coll;
        mlsl_sched_prologue prologue;
        mlsl_sched_epilogue epilogue;
        mlsl_sched_update_postponed_fields update_fields;
        mlsl_sched_tensor_comm tensor_comm;
    } u;
};

enum mlsl_sched_memory_type
{
    mlsl_sched_memory_buffer     = 0,
    mlsl_sched_memory_registered = 1
};

struct mlsl_sched_memory
{
    mlsl_sched_memory_type type;
    void *ptr;

    mlsl_sched_memory *next;
    mlsl_sched_memory *prev;
};

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

struct mlsl_sched_coll_param
{
    mlsl_coll_type ctype;
    void *buf;
    const void *send_buf;
    void *recv_buf;
    size_t count;
    size_t send_count;
    size_t *recv_counts;
    mlsl_datatype_internal_t dtype;
    mlsl_reduction_t reduction;
    size_t root;
    mlsl_comm *comm;
};

struct mlsl_sched_postponed_fields
{
    void* buf;
    size_t count;
    mlsl_datatype_internal dtype;
    void* ctx;
};

struct mlsl_sched
{
    int first_progress;
    mlsl_sched_queue_bin *bin;
    mlsl_sched_coll_param coll_param;
    mlsl_sched_coll_attr coll_attr;
    size_t size;               /* capacity (in entries) of the entries array */
    size_t idx;                /* index into entries array of first yet-outstanding entry */ /* index to start */
    size_t num_entries;        /* number of populated entries, num_entries <= size */
    mlsl_sched_id_t sched_id;   /* sequence number of the schedule in the communicator */
    mlsl_request *req;
    mlsl_sched_entry *entries;
    mlsl_sched_memory *persistent_memory;

    mlsl_sched_postponed_fields postponed_fields;

    mlsl_sched **partial_scheds;
    size_t partial_sched_count;

    mlsl_sched* root;

    mlsl_sched *next;   /* linked-list next pointer */
    mlsl_sched *prev;   /* linked-list prev pointer */
};

mlsl_status_t mlsl_sched_create(mlsl_sched **sched);
mlsl_status_t mlsl_sched_commit(mlsl_sched *sched);
mlsl_status_t mlsl_sched_start(mlsl_sched *sched, mlsl_request **req);
mlsl_status_t mlsl_sched_free(mlsl_sched *sched);

mlsl_status_t mlsl_sched_add_send(mlsl_sched *sched, const void *buf, size_t count,
                                  mlsl_datatype_internal_t dtype, size_t dest);
mlsl_status_t mlsl_sched_add_recv(mlsl_sched *sched, void *buf, size_t count,
                                  mlsl_datatype_internal_t dtype, size_t src);
mlsl_status_t mlsl_sched_add_reduce(mlsl_sched *sched, const void *in_buf, size_t in_count,
                                    void *inout_buf, size_t *out_count,
                                    mlsl_datatype_internal_t dtype, mlsl_reduction_t reduction);

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
                                         size_t *out_count, mlsl_datatype_internal_t dtype,
                                         mlsl_reduction_t op, size_t src,
                                         void* comm_buf);

mlsl_status_t mlsl_sched_add_copy(mlsl_sched *sched, const void *in_buf,
                                           void *out_buf, size_t count, mlsl_datatype_internal_t dtype);
mlsl_status_t mlsl_sched_add_barrier(mlsl_sched *sched);
mlsl_status_t mlsl_sched_add_sync(mlsl_sched *sched, mlsl_sched_entry **sync_entry, int* entry_idx);
mlsl_status_t mlsl_sched_sync_schedules(mlsl_sched **scheds, size_t count);

mlsl_status_t mlsl_sched_add_collective(mlsl_sched *sched, mlsl_coll_type ctype,
                                        const void* send_buf, void* recv_buf, size_t count,
                                        mlsl_datatype_internal_t dtype, mlsl_reduction_t reduction);
mlsl_status_t mlsl_sched_add_prologue(mlsl_sched *sched, mlsl_prologue_fn_t fn, const void* in_buf,
                                      size_t in_count, mlsl_datatype_internal_t in_dtype,
                                      void** out_buf, size_t* out_count, mlsl_datatype_internal_t out_dtype);
mlsl_status_t mlsl_sched_add_epilogue(mlsl_sched *sched, mlsl_epilogue_fn_t fn, const void* in_buf, size_t in_count,
                                      mlsl_datatype_internal_t in_dtype, void* out_buf, size_t* out_count,
                                      size_t expected_out_count, mlsl_datatype_internal_t out_dtype);
mlsl_status_t mlsl_sched_add_update_postponed_fields(mlsl_sched *sched, size_t part_idx, size_t part_count);

mlsl_status_t mlsl_sched_add_tensor_comm_create_entry(mlsl_sched* sched,
                                                      out_of_order::ooo_match* ooo_handler,
                                                      const char* tensor_name_buffer);

mlsl_status_t mlsl_sched_bcast(void *buf, size_t count, mlsl_datatype_internal_t dtype,
                               size_t root, mlsl_comm* comm, mlsl_sched **sched);
mlsl_status_t mlsl_sched_reduce(const void *send_buf, void *recv_buf, size_t count, mlsl_datatype_internal_t dtype,
                                mlsl_reduction_t reduction, size_t root, mlsl_comm* comm, mlsl_sched **sched);
mlsl_status_t mlsl_sched_allreduce(const void *send_buf, void *recv_buf, size_t count, mlsl_datatype_internal_t dtype,
                                   mlsl_reduction_t reduction, mlsl_comm* comm, mlsl_sched **sched);
mlsl_status_t mlsl_sched_allgatherv(const void *send_buf, size_t send_count, void *recv_buf, size_t *recv_counts,
                                    mlsl_datatype_internal_t dtype, mlsl_comm* comm, mlsl_sched **sched);
mlsl_status_t mlsl_sched_barrier(mlsl_comm* comm, mlsl_sched **sched);

mlsl_status_t mlsl_sched_tensor_bcast(mlsl_comm* comm, mlsl_sched** sched, bool temporal);

mlsl_status_t mlsl_sched_progress(mlsl_sched_queue_bin *bin, size_t sched_count, size_t *processed_sched_count);
mlsl_status_t mlsl_sched_clone(mlsl_sched *orig, mlsl_sched **clone);
mlsl_status_t mlsl_sched_adjust(mlsl_sched *sched, size_t partition_idx, size_t partition_count);
mlsl_status_t mlsl_sched_adjust_entries(mlsl_sched *sched, size_t partition_idx, size_t partition_count);
mlsl_status_t mlsl_sched_adjust_tag(mlsl_sched *sched);
mlsl_status_t mlsl_sched_dump(mlsl_sched *sched, const char *name);
mlsl_status_t mlsl_sched_reset(mlsl_sched *sched);

mlsl_status_t mlsl_sched_add_persistent_memory(mlsl_sched *sched, mlsl_sched_memory_type type, void *ptr);
mlsl_status_t mlsl_sched_free_persistent_memory(mlsl_sched *sched);

mlsl_status_t mlsl_sched_set_coll_attr(mlsl_sched *sched, const mlsl_coll_attr_t *attr);

size_t mlsl_sched_get_priority(mlsl_sched *sched);

void mlsl_update_request_reference(mlsl_sched *s, mlsl_request **req);

void mlsl_sched_prepare(mlsl_sched *sched, bool dump);
