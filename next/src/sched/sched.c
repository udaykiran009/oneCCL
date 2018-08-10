#include "comp.h"
#include "global.h"
#include "log.h"
#include "sched.h"
#include "utils.h"

#include <stdio.h>

#define MLSL_SCHED_INITIAL_ENTRIES (16)

static const char *mlsl_sched_entry_type_to_str(mlsl_sched_entry_type type)
{
    switch (type) {
        case mlsl_sched_entry_send:
            return "SEND";
        case mlsl_sched_entry_recv:
            return "RECV";
        case mlsl_sched_entry_reduce:
            return "REDUCE";
        case mlsl_sched_entry_compute:
            return "COMPUTE";
        case mlsl_sched_entry_copy:
            return "COPY";
        case mlsl_sched_entry_nop:
            return "NOP";
        default:
            MLSL_ASSERTP(0);
            return "(out of range)";
    }
}

mlsl_status_t mlsl_sched_dump(mlsl_sched *s)
{
    if (!env_data.sched_dump) return mlsl_status_success;

    size_t i;
    char buf[4096] = { 0 };
    size_t buf_offset = 0;

    buf_offset += sprintf(buf + buf_offset, "\n--------------------------------\n");
    if (s) {
        buf_offset += sprintf(buf + buf_offset, "sched: %p, sz %zu, start_idx %zu, num_entries %zu, tag %d, req %p, entries %p\n",
                              s, s->size, s->idx, s->num_entries, s->tag, s->req, s->entries);

        for (i = 0; i < s->num_entries; ++i) {
            mlsl_sched_entry *e = &(s->entries[i]);
            buf_offset += sprintf(buf + buf_offset, "entries[%zu]: type %-7s, status %d, is_barrier %-5s, ",
                                  i, mlsl_sched_entry_type_to_str(e->type),
                                  e->status, (e->is_barrier ? "TRUE" : "FALSE"));

            switch (e->type) {
                case mlsl_sched_entry_send:
                    buf_offset += sprintf(buf + buf_offset, "buf %p, cnt %zu, dt %d, dst %zu, comm %p, req %p\n",
                                          e->u.send.buf, e->u.send.count, e->u.send.dtype,
                                          e->u.send.dest, e->u.send.comm, &e->u.send.req);
                    break;
                case mlsl_sched_entry_recv:
                    buf_offset += sprintf(buf + buf_offset, "buf %p, cnt %zu, dt %d, src %zu, comm %p, req %p\n",
                                          e->u.recv.buf, e->u.recv.count, e->u.recv.dtype,
                                          e->u.recv.src, e->u.recv.comm, &e->u.recv.req);
                    break;
                case mlsl_sched_entry_reduce:
                    buf_offset += sprintf(buf + buf_offset, "in_buf %p, in_cnt %zu, inout_buf %p, out_cnt %p, dt %d, op %d\n",
                                          e->u.reduce.in_buf, e->u.reduce.in_count,
                                          e->u.reduce.inout_buf, e->u.reduce.out_count,
                                          e->u.reduce.dtype, e->u.reduce.op);
                    break;
                case mlsl_sched_entry_copy:
                    buf_offset += sprintf(buf + buf_offset, "in_buf %p, out_buf %p, cnt %zu, dt %d\n",
                                          e->u.copy.in_buf, e->u.copy.out_buf,
                                          e->u.copy.count, e->u.copy.dtype);
                    break;
                case mlsl_sched_entry_compute:
                    buf_offset += sprintf(buf + buf_offset, "type %d, in_buf %p, in_cnt %zu, out_buf %p, out_cnt %p, dt %d\n",
                                          e->u.compute.type,
                                          e->u.compute.in_buf, e->u.compute.in_count,
                                          e->u.compute.out_buf, e->u.compute.out_count,
                                          e->u.compute.dtype);
                    break;
                case mlsl_sched_entry_nop:
                    break;
                default:
                    MLSL_ASSERTP(0);
            }
        }
    }
    buf_offset += sprintf(buf + buf_offset, "--------------------------------\n");
    MLSL_LOG(INFO, "%s", buf);

    return mlsl_status_success;
}

static void mlsl_sched_entry_clone(mlsl_sched_entry *orig, mlsl_sched_entry *copy)
{
    // memcpy is enough for now
    // TODO: handle out_size summarization, allocation of internal buffers and etc
    memcpy(copy, orig, sizeof(mlsl_sched_entry));
}

#define MLSL_GET_COUNT_AND_OFFSET(count, dtype, part_idx, part_count, out_count, out_offset) \
    do {                                                                                     \
        out_count = count / part_count;                                                      \
        out_offset = part_idx * out_count * mlsl_get_dtype_size(dtype);                      \
        if (part_idx == (part_count - 1)) out_count += count % part_count;                   \
    } while (0)

#define MLSL_ADJUST_PTR(ptr, offset) ptr = (char*)ptr + offset

static void mlsl_sched_entry_adjust(mlsl_sched_entry *entry, size_t partition_idx, size_t partition_count)
{
    size_t adjust_count, adjust_offset;

    switch (entry->type) {
        case mlsl_sched_entry_send:
            MLSL_GET_COUNT_AND_OFFSET(entry->u.send.count, entry->u.send.dtype,
                                             partition_idx, partition_count,
                                             adjust_count, adjust_offset);
            entry->u.send.count = adjust_count;
            MLSL_ADJUST_PTR(entry->u.send.buf, adjust_offset);
            break;
        case mlsl_sched_entry_recv:
            MLSL_GET_COUNT_AND_OFFSET(entry->u.recv.count, entry->u.recv.dtype,
                                      partition_idx, partition_count,
                                      adjust_count, adjust_offset);
            entry->u.recv.count = adjust_count;
            MLSL_ADJUST_PTR(entry->u.recv.buf, adjust_offset);
            break;
        case mlsl_sched_entry_reduce:
            MLSL_GET_COUNT_AND_OFFSET(entry->u.reduce.in_count, entry->u.reduce.dtype,
                                      partition_idx, partition_count,
                                      adjust_count, adjust_offset);
            entry->u.reduce.in_count = adjust_count;
            MLSL_ADJUST_PTR(entry->u.reduce.in_buf, adjust_offset);
            MLSL_ADJUST_PTR(entry->u.reduce.inout_buf, adjust_offset);
            break;
        case mlsl_sched_entry_copy:
            MLSL_GET_COUNT_AND_OFFSET(entry->u.copy.count, entry->u.copy.dtype,
                                      partition_idx, partition_count,
                                      adjust_count, adjust_offset);
            entry->u.copy.count = adjust_count;
            MLSL_ADJUST_PTR(entry->u.copy.in_buf, adjust_offset);
            MLSL_ADJUST_PTR(entry->u.copy.out_buf, adjust_offset);
            break;
        case mlsl_sched_entry_compute:
            MLSL_GET_COUNT_AND_OFFSET(entry->u.compute.in_count, entry->u.compute.dtype,
                                      partition_idx, partition_count,
                                      adjust_count, adjust_offset);
            entry->u.compute.in_count = adjust_count;
            MLSL_ADJUST_PTR(entry->u.compute.in_buf, adjust_offset);
            MLSL_ADJUST_PTR(entry->u.compute.out_buf, adjust_offset);
            break;
        case mlsl_sched_entry_nop:
            break;
        default:
            MLSL_ASSERTP(0);
    }
    return;
}

mlsl_status_t mlsl_sched_next_tag(mlsl_comm *comm, int *tag)
{
    int tag_ub = global_data.tag_ub;
    *tag = comm->next_sched_tag;
    ++comm->next_sched_tag;

    /* wrap the tag values around to the start */
    if (comm->next_sched_tag == tag_ub) {
        comm->next_sched_tag = MLSL_TAG_FIRST;
    }

    return mlsl_status_success;
}

/* initiates the schedule entry "e" in the NBC described by "s", where
 * "e" is at "idx" in "s".  This means posting nonblocking sends/recvs,
 * performing reductions, calling callbacks, etc. */
mlsl_status_t mlsl_sched_start_entry(mlsl_sched *s, size_t idx, mlsl_sched_entry *e)
{
    mlsl_status_t status = mlsl_status_success;
    atl_status_t atl_status = atl_status_success;
    mlsl_status_t comp_status = mlsl_status_success;

    MLSL_ASSERT_FMT(e->status == mlsl_sched_entry_status_not_started, "entry is already started");

    switch (e->type) {
        case mlsl_sched_entry_send:
            MLSL_LOG(DEBUG, "starting SEND entry %zu", idx);

            atl_status = atl_comm_send(s->bin->comm_ctx, e->u.send.buf,
                                       e->u.send.count * mlsl_get_dtype_size(e->u.send.dtype),
                                       e->u.send.dest, s->tag, &e->u.send.req);

            if (unlikely(atl_status != atl_status_success)) {
                e->status = mlsl_sched_entry_status_failed;
                MLSL_LOG(DEBUG, "Sched SEND failed. atl_status: %d", atl_status);
            } else {
                e->status = mlsl_sched_entry_status_started;
            }
            break;
        case mlsl_sched_entry_recv:
            MLSL_LOG(DEBUG, "starting RECV entry %zu", idx);

            atl_status = atl_comm_recv(s->bin->comm_ctx, e->u.recv.buf,
                                       e->u.recv.count * mlsl_get_dtype_size(e->u.recv.dtype),
                                       e->u.recv.src, s->tag, &e->u.recv.req);

            if (unlikely(atl_status != atl_status_success)) {
                e->status = mlsl_sched_entry_status_failed;
                MLSL_LOG(DEBUG, "Sched RECV failed. atl_status: %d", atl_status);
            } else {
                e->status = mlsl_sched_entry_status_started;
            }
            break;
        case mlsl_sched_entry_reduce:
            MLSL_LOG(DEBUG, "starting REDUCE entry %zu", idx);
            comp_status = mlsl_comp_reduce(e->u.reduce.in_buf, e->u.reduce.in_count,
                                           e->u.reduce.inout_buf, e->u.reduce.out_count,
                                           e->u.reduce.dtype, e->u.reduce.op);
            MLSL_ASSERTP(comp_status == mlsl_status_success);
            // dtype is not builtin - release ref
            // op is not builtin - release ref
            e->status = mlsl_sched_entry_status_complete;
            break;
        case mlsl_sched_entry_copy:
            MLSL_LOG(DEBUG, "starting COPY entry %zu", idx);
            comp_status = mlsl_comp_copy(e->u.copy.in_buf, e->u.copy.out_buf, e->u.copy.count, e->u.copy.dtype);
            MLSL_ASSERTP(comp_status == mlsl_status_success);
            // indtype is not builtin - release ref
            // outdtype is not builtin - release ref
            e->status = mlsl_sched_entry_status_complete;
            break;
        case mlsl_sched_entry_nop:
            MLSL_LOG(DEBUG, "starting NOP entry %zu", idx);
            /* nothing to be done */
            break;
        case mlsl_sched_entry_compute:
            MLSL_LOG(DEBUG, "starting COMPUTE entry %zu", idx);
            if (e->u.compute.type == mlsl_sched_compute_1i1o)
            {
                status = e->u.compute.u.fn_1i1o(e->u.compute.in_buf, e->u.compute.in_count,
                                                e->u.compute.out_buf, e->u.compute.out_count,
                                                e->u.compute.dtype);
                e->status = mlsl_sched_entry_status_complete;
            } else {
                MLSL_LOG(DEBUG, "unknown callback type, e->u.compute.type=%d", e->u.compute.type);
                e->status = mlsl_sched_entry_status_failed;
            }
            break;
        default:
            MLSL_LOG(DEBUG, "unknown entry type, e->type=%d", e->type);
            e->status = mlsl_sched_entry_status_failed;
            break;
    }

    return status;
}

/* Posts or performs any NOT_STARTED operations in the given schedule that are
 * permitted to be started.  That is, this routine will respect schedule
 * barriers appropriately. */
static mlsl_status_t mlsl_sched_continue(mlsl_sched *s)
{
    mlsl_status_t status = mlsl_status_success;
    size_t i;

    for (i = s->idx; i < s->num_entries; ++i)
    {
        mlsl_sched_entry *e = &s->entries[i];

        if (e->status == mlsl_sched_entry_status_not_started) {
            status = mlsl_sched_start_entry(s, i, e);
            /* sched entries list can be reallocated inside callback */
            e = &s->entries[i];
            MLSL_ASSERTP(status == mlsl_status_success);
        }

        /* _start_entry may have completed the operation, but won't update s->idx */
        if (i == s->idx && e->status >= mlsl_sched_entry_status_complete) {
            ++s->idx;   /* this is valid even for barrier entries */
        }

        /* watch the indexing, s->idx might have been incremented above, so
         * ||-short-circuit matters here */
        if (e->is_barrier && (e->status < mlsl_sched_entry_status_complete || (s->idx != i + 1))) {
            /* we've hit a barrier but outstanding operations before this
             * barrier remain, so we cannot proceed past the barrier */
            break;
        }
    }

    return status;
}

/* creates a new opaque schedule object and returns a handle to it in (*sp) */
mlsl_status_t mlsl_sched_create(mlsl_sched **sp)
{
    mlsl_status_t status = mlsl_status_success;

    mlsl_sched *s = NULL;

    /* this mem will be freed by the progress engine when the request is completed */
    s = MLSL_CALLOC(sizeof(mlsl_sched), "schedule");

    s->size = MLSL_SCHED_INITIAL_ENTRIES;
    s->idx = 0;
    s->num_entries = 0;
    s->tag = MLSL_TAG_UNDEFINED;
    s->req = NULL;
    s->entries = NULL;
    s->next = NULL;     /* only needed for sanity checks */
    s->prev = NULL;     /* only needed for sanity checks */

    /* this mem will be freed by the progress engine when the request is completed */
    s->entries = MLSL_CALLOC(MLSL_SCHED_INITIAL_ENTRIES * sizeof(mlsl_sched_entry),
                             "schedule entries");

    *sp = s;
    return status;
}

/* clones orig and returns a handle to the new schedule */
mlsl_status_t mlsl_sched_clone(mlsl_sched *orig, mlsl_sched **clone)
{
    mlsl_sched *s = MLSL_CALLOC(sizeof(mlsl_sched), "schedule");

    s->size = orig->size;
    s->idx = 0;
    s->num_entries = orig->num_entries;
    s->tag = orig->tag;
    s->req = orig->req; // cloned scheds point to origin req for completion notification
    s->entries = NULL;
    s->next = NULL;
    s->prev = NULL;
    s->entries = MLSL_CALLOC(s->size * sizeof(mlsl_sched_entry),
                             "schedule entries");

    size_t idx;
    for (idx = 0; idx < s->num_entries; idx++)
    {
        mlsl_sched_entry_clone(&(orig->entries[idx]), &(s->entries[idx]));
    }

    *clone = s;

    return mlsl_status_success;
}

mlsl_status_t mlsl_sched_adjust(mlsl_sched *sched, size_t partition_idx, size_t partition_count)
{
    size_t idx;
    for (idx = 0; idx < sched->num_entries; idx++)
    {
        mlsl_sched_entry_adjust(&(sched->entries[idx]), partition_idx, partition_count);
    }

    int tag;
    mlsl_sched_next_tag(global_data.comm, &tag);
    sched->tag = tag;

    return mlsl_status_success;
}

mlsl_status_t mlsl_sched_reset(mlsl_sched *sched)
{
    size_t idx;
    sched->idx = 0;
    for (idx = 0; idx < sched->num_entries; idx++)
    {
        sched->entries[idx].status = mlsl_sched_entry_status_not_started;
    }
    return mlsl_status_success;
}

mlsl_status_t mlsl_sched_start(mlsl_sched *s, mlsl_request **req)
{
    /* sanity check the schedule */
    MLSL_ASSERT(s->num_entries <= s->size);
    MLSL_ASSERT(s->num_entries == 0 || s->idx < s->num_entries);
    MLSL_ASSERT(s->req != NULL);
    MLSL_ASSERT(s->next == NULL);
    MLSL_ASSERT(s->prev == NULL);
    MLSL_ASSERT(s->entries != NULL);

    mlsl_executor_start(global_data.executor, s);
    MLSL_LOG(DEBUG, "started schedule %p", s);

    *req = s->req;

    return mlsl_status_success;
}

/* idx and e are permitted to be NULL */
static mlsl_status_t mlsl_sched_add_entry(mlsl_sched *s, int *idx, mlsl_sched_entry **e)
{
    mlsl_status_t status = mlsl_status_success;
    int i;
    mlsl_sched_entry *ei;

    MLSL_ASSERT(s->entries != NULL);
    MLSL_ASSERT(s->size > 0);

    if (s->num_entries == s->size) {
        /* need to grow the entries array */
        s->entries = MLSL_REALLOC(s->entries, s->size * sizeof(mlsl_sched_entry),
                                  2 * s->size * sizeof(mlsl_sched_entry), CACHELINE_SIZE,
                                  "schedule entries");
        MLSL_ASSERTP_FMT(s->entries, "**nomem");
        s->size *= 2;
    }

    i = s->num_entries++;
    ei = &s->entries[i];

    if (idx != NULL)
        *idx = i;
    if (e != NULL)
        *e = ei;

    return status;
}

mlsl_status_t mlsl_sched_add_send(mlsl_sched *sched, const void *buf, size_t count,
                                  mlsl_data_type_t dtype, size_t dest)
{
    mlsl_status_t status = mlsl_status_success;
    mlsl_sched_entry *e = NULL;

    status = mlsl_sched_add_entry(sched, NULL, &e);
    MLSL_ASSERTP(status == mlsl_status_success);

    e->type = mlsl_sched_entry_send;
    e->status = mlsl_sched_entry_status_not_started;
    e->is_barrier = 0;

    e->u.send.buf = buf;
    e->u.send.count = count;
    e->u.send.dtype = dtype;
    e->u.send.dest = dest;
    // TODO_ATL: return back when atl_request_t will be replaced by handle
    //e->u.send.req = NULL;      /* will be populated by _start_entry */
    e->u.send.comm = global_data.comm;

    mlsl_comm_add_ref(e->u.send.comm);

    return status;
}

mlsl_status_t mlsl_sched_add_recv(mlsl_sched *sched, void *buf, size_t count,
                                  mlsl_data_type_t dtype, size_t src)
{
    mlsl_status_t status = mlsl_status_success;
    mlsl_sched_entry *e = NULL;

    status = mlsl_sched_add_entry(sched, NULL, &e);
    MLSL_ASSERTP(status == mlsl_status_success);

    e->type = mlsl_sched_entry_recv;
    e->status = mlsl_sched_entry_status_not_started;
    e->is_barrier = 0;

    e->u.recv.buf = buf;
    e->u.recv.count = count;
    e->u.recv.dtype = dtype;
    e->u.recv.src = src;
    //e->u.recv.req = NULL;      /* will be populated by _start_entry */
    e->u.recv.comm = global_data.comm;

    mlsl_comm_add_ref(e->u.recv.comm);

    return status;
}

mlsl_status_t mlsl_sched_add_reduce(mlsl_sched *sched, const void *in_buf, size_t in_count,
                                    void *inout_buf, size_t *out_count,
                                    mlsl_data_type_t dtype, mlsl_reduction_t op)
{
    mlsl_status_t status = mlsl_status_success;
    mlsl_sched_entry *e = NULL;
    mlsl_sched_reduce *reduce = NULL;

    status = mlsl_sched_add_entry(sched, NULL, &e);
    MLSL_ASSERTP(status == mlsl_status_success);

    e->type = mlsl_sched_entry_reduce;
    e->status = mlsl_sched_entry_status_not_started;
    e->is_barrier = 0;
    reduce = &e->u.reduce;

    reduce->in_buf = in_buf;
    reduce->in_count = in_count;
    reduce->inout_buf = inout_buf;
    reduce->out_count = out_count;
    reduce->dtype = dtype;
    reduce->op = op;

    // if op is not builtin - add ref

    return status;
}

mlsl_status_t mlsl_sched_add_copy(mlsl_sched *sched, const void *in_buf,
                                  void *out_buf, size_t count, mlsl_data_type_t dtype)
{
    mlsl_status_t status = mlsl_status_success;
    mlsl_sched_entry *e = NULL;
    mlsl_sched_copy *copy = NULL;

    status = mlsl_sched_add_entry(sched, NULL, &e);
    MLSL_ASSERTP(status == mlsl_status_success);

    e->type = mlsl_sched_entry_copy;
    e->status = mlsl_sched_entry_status_not_started;
    e->is_barrier = 0;
    copy = &e->u.copy;

    copy->in_buf = in_buf;
    copy->out_buf = out_buf;
    copy->count = count;
    copy->dtype = dtype;

    return status;
}

/* require that all previously added ops are complete before subsequent ops
 * may begin to execute */
mlsl_status_t mlsl_sched_add_barrier(mlsl_sched *sched)
{
    mlsl_status_t status = mlsl_status_success;

    /* mark the previous entry as a barrier unless we're at the beginning, which
     * would be a pointless barrier */
    if (sched->num_entries > 0) {
        sched->entries[sched->num_entries - 1].is_barrier = 1;
    }

    return status;
}

mlsl_status_t mlsl_sched_add_compute_1i1o(mlsl_sched *sched, mlsl_sched_compute_1i1o_fn_t fn_ptr,
                                          const void* in_buf, size_t in_count,
                                          void *out_buf, size_t *out_count,
                                          mlsl_data_type_t dtype)
{
    mlsl_status_t status = mlsl_status_success;
    mlsl_sched_entry *e = NULL;
    mlsl_sched_compute *compute = NULL;

    status = mlsl_sched_add_entry(sched, NULL, &e);
    MLSL_ASSERTP(status == mlsl_status_success);

    e->type = mlsl_sched_entry_compute;
    e->status = mlsl_sched_entry_status_not_started;
    e->is_barrier = 0;
    compute = &e->u.compute;

    compute->type = mlsl_sched_compute_1i1o;
    compute->u.fn_1i1o = fn_ptr;
    compute->in_buf = in_buf;
    compute->in_count = in_count;
    compute->out_buf = out_buf;
    compute->out_count = out_count;
    compute->dtype = dtype;

    // TODO: add send_defer and callback primitives for resource cleanup

    return status;
}

mlsl_status_t mlsl_sched_progress(mlsl_sched_queue_bin *bin, size_t sched_count, size_t *processed_sched_count)
{
    mlsl_status_t status = mlsl_status_success;
    size_t i;
    mlsl_sched *s;
    mlsl_sched *tmp;
    size_t sched_idx = 0;

    if (processed_sched_count)
        *processed_sched_count = 0;

    MLSL_LOG(TRACE, "bin %p, elems %p, sched_count %zu",
             bin, bin->elems, sched_count);

    /* ensure communication progress */
    atl_status_t atl_status = atl_comm_poll(bin->comm_ctx);
    MLSL_ASSERTP(atl_status == atl_status_success);

    /* update schedule states */
    MLSL_DLIST_FOREACH_SAFE(bin->elems, s, tmp) {
        for (i = s->idx; i < s->num_entries; ++i) {
            mlsl_sched_entry *e = &s->entries[i];

            if ((i == 0) && (e->status == mlsl_sched_entry_status_not_started))
            {
                MLSL_LOG(DEBUG, "do initial mlsl_sched_continue");
                status = mlsl_sched_continue(s);
                MLSL_ASSERTP(status == mlsl_status_success);
            }

            int req_status;
            switch (e->type) {
                case mlsl_sched_entry_send:
                    if (e->status < mlsl_sched_entry_status_complete) {
                        atl_comm_check(bin->comm_ctx, &req_status, &e->u.send.req);
                        if (req_status) {
                            MLSL_LOG(DEBUG, "completed SEND entry %zu, sreq=%p", i, &e->u.send.req);
                            e->status = mlsl_sched_entry_status_complete;
                            mlsl_comm_release_ref(e->u.send.comm);
                        }
                    }
                    break;
                case mlsl_sched_entry_recv:
                    if (e->status < mlsl_sched_entry_status_complete) {
                        atl_comm_check(bin->comm_ctx, &req_status, &e->u.recv.req);
                        if (req_status) {
                            MLSL_LOG(DEBUG, "completed RECV entry %zu, rreq=%p", i, &e->u.recv.req);
                            e->status = mlsl_sched_entry_status_complete;
                            mlsl_comm_release_ref(e->u.recv.comm);
                        }
                    }
                    break;
                default:
                    /* all other entry types don't have any sub-requests that
                     * need to be checked */
                    break;
            }

            if (i == s->idx && e->status >= mlsl_sched_entry_status_complete) {
                ++s->idx;
                MLSL_LOG(DEBUG, "completed OTHER entry %zu", i);
                if (e->is_barrier) {
                    /* post/perform the next round of operations */
                    status = mlsl_sched_continue(s);
                    MLSL_ASSERTP(status == mlsl_status_success);
                }
            } else if (e->is_barrier && e->status < mlsl_sched_entry_status_complete) {
                /* don't process anything after this barrier entry */
                break;
            }
        }

        if (s->idx == s->num_entries) {
            MLSL_LOG(DEBUG, "completing and dequeuing: sched %p, req %p", s, s->req);

            /* dequeue this schedule, it's complete */
            mlsl_sched_queue_remove(bin->queue, bin, s);

            status = mlsl_request_complete(s->req);
            MLSL_ASSERTP(status == mlsl_status_success);

            if (processed_sched_count)
                (*processed_sched_count)++;
        }

        sched_idx++;
        if (sched_idx == sched_count) break;
    }

    return status;
}

mlsl_status_t mlsl_sched_commit_with_type(mlsl_sched *sched, mlsl_sched_type type)
{
    int tag;
    mlsl_sched_next_tag(global_data.comm, &tag);
    sched->tag = tag;

    mlsl_request *req;
    mlsl_request_create(&req);
    req->sched = sched;
    sched->req = req;

    sched->type = type;

    MLSL_LOG(DEBUG, "sched %p, tag %d, req %p, type %d",
             sched, tag, req, type);

    return mlsl_status_success;
}

mlsl_status_t mlsl_sched_commit(mlsl_sched *sched)
{
    mlsl_sched_commit_with_type(sched, mlsl_sched_persistent);
    return mlsl_status_success;
}

mlsl_status_t mlsl_sched_free(mlsl_sched *sched)
{
    mlsl_sched_cache_free_entry(global_data.sched_cache, sched);
    mlsl_request_free(sched->req);
    MLSL_FREE(sched->entries);
    MLSL_FREE(sched);
    return mlsl_status_success;
}

mlsl_status_t mlsl_sched_set_prologue(mlsl_sched *sched, mlsl_sched_prolog_fn_t fn)
{
    return mlsl_status_unimplemented;
}

mlsl_status_t mlsl_sched_set_epilogue(mlsl_sched *sched, mlsl_sched_epilog_fn_t fn)
{
    return mlsl_status_unimplemented;
}

mlsl_status_t mlsl_sched_set_reduction(mlsl_sched *sched, mlsl_sched_reduction_fn_t fn)
{
    return mlsl_status_unimplemented;
}

mlsl_status_t mlsl_sched_allreduce(
    void *send_buf,
    void *recv_buf,
    size_t count,
    mlsl_data_type_t dtype,
    mlsl_reduction_t reduction,
    mlsl_sched_t **sched)
{
    return mlsl_status_unimplemented;
}

mlsl_status_t mlsl_sched_queue_create(size_t max_bins, atl_comm_t **comm_ctxs, mlsl_sched_queue **queue)
{
    MLSL_ASSERTP(max_bins <= MAX_SCHED_BINS);
    mlsl_sched_queue *q = MLSL_CALLOC(sizeof(mlsl_sched_queue), "schedule queue");
    mlsl_fastlock_init(&q->lock);
    for (size_t idx = 0; idx < max_bins; idx++)
    {
        q->bins[idx].queue = q;
        q->bins[idx].comm_ctx = comm_ctxs[idx];
        MLSL_LOG(DEBUG, "comm_ctxs[%zu]: %p", idx, comm_ctxs[idx]);
    }
    q->max_bins = max_bins;

    *queue = q;

    return mlsl_status_success;
}

mlsl_status_t mlsl_sched_queue_free(mlsl_sched_queue *queue)
{
    mlsl_fastlock_destroy(&queue->lock);
    MLSL_ASSERTP(queue->used_bins == 0);
    MLSL_FREE(queue);

    return mlsl_status_success;
}

mlsl_status_t mlsl_sched_queue_add(mlsl_sched_queue *queue, mlsl_sched *sched, size_t priority)
{
    mlsl_fastlock_acquire(&queue->lock);
    mlsl_sched_queue_bin *bin = &(queue->bins[priority]);
    if (!bin->elems) queue->used_bins++;
    MLSL_DLIST_APPEND(bin->elems, sched);
    sched->bin = bin;
    bin->elem_count++;
    queue->max_priority = MAX(queue->max_priority, priority);
    mlsl_fastlock_release(&queue->lock);

    return mlsl_status_success;
}

mlsl_status_t mlsl_sched_queue_remove(mlsl_sched_queue *queue, mlsl_sched_queue_bin *bin, mlsl_sched *sched)
{
    MLSL_LOG(DEBUG, "queue %p, bin %p, elems %p, sched %p, count %zu",
             queue, bin, bin->elems, sched, bin->elem_count);

    mlsl_fastlock_acquire(&queue->lock);
    MLSL_DLIST_DELETE(bin->elems, sched);
    bin->elem_count--;
    if (!bin->elems)
    {
        queue->used_bins--;
        if (queue->used_bins == 0) queue->max_priority = 0;
        else
        {
            size_t bin_idx = (bin->priority - 1 + queue->max_bins) % queue->max_bins;
            while (!queue->bins[bin_idx].elems)
            {
                bin_idx = (bin_idx - 1 + queue->max_bins) % queue->max_bins;
            }
            queue->max_priority = queue->bins[bin_idx].priority;
        }

    }
    mlsl_fastlock_release(&queue->lock);

    return mlsl_status_success;
}

mlsl_status_t mlsl_sched_queue_peek(mlsl_sched_queue *queue, mlsl_sched_queue_bin **bin, size_t *count)
{
    mlsl_fastlock_acquire(&queue->lock);
    if (queue->used_bins > 0)
    {
        *bin = &(queue->bins[queue->max_priority]);
        *count = (*bin)->elem_count;
    }
    else
    {
        *bin = NULL;
        *count = 0;
    }
    mlsl_fastlock_release(&queue->lock);

    return mlsl_status_success;
}
