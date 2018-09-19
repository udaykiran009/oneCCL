#include "comp.h"
#include "global.h"
#include "log.h"
#include "sched.h"
#include "utils.h"

#include <immintrin.h>
#include <stdio.h>

#define MLSL_SCHED_INITIAL_ENTRIES (16)

mlsl_priority_mode mlsl_sched_get_priority(mlsl_sched *sched)
{
    static size_t lifo_priority = 0;
    size_t priority;
    switch (env_data.priority_mode)
    {
        case mlsl_priority_none:
            priority = 0;
            break;
        case mlsl_priority_direct:
            if (sched->coll_param.ctype == mlsl_coll_barrier)
                priority = (MLSL_SCHED_QUEUE_MAX_BINS - 1);
            else
                priority = sched->coll_attr.priority;
            MLSL_ASSERT(priority >= 0 && priority < MLSL_SCHED_QUEUE_MAX_BINS);
            break;
        case mlsl_priority_lifo:
            priority = sched->coll_attr.priority;
            MLSL_ASSERT(priority >= 0);
            break;
        default:
            MLSL_ASSERTP_FMT(0, "unexpected priority_mode %d", env_data.priority_mode);
            break;
    }
    MLSL_LOG(DEBUG, "mlsl_sched_get_priority: s %p, p %zu", sched, priority);
    return priority;
}

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
        case mlsl_sched_entry_sync:
            return "SYNC";
        case mlsl_sched_entry_nop:
            return "NOP";
        default:
            MLSL_ASSERT_FMT(0, "unexpected entry_type %d", type);
            return "(out of range)";
    }
}

static const char *mlsl_dtype_to_str(mlsl_data_type_t type)
{
    switch (type) {
        case mlsl_dtype_char:
            return "CHAR";
        case mlsl_dtype_int:
            return "INT";
        case mlsl_dtype_bfp16:
            return "BFP16";
        case mlsl_dtype_float:
            return "FLOAT";
        case mlsl_dtype_double:
            return "DOUBLE";
        case mlsl_dtype_int64:
            return "INT64";
        case mlsl_dtype_uint64:
            return "UINT64";
        default:
            MLSL_ASSERT_FMT(0, "unexpected dtype %d", type);
            return "(out of range)";
    }
}

static const char *mlsl_reduction_to_str(mlsl_reduction_t type)
{
    switch (type) {
        case mlsl_reduction_sum:
            return "SUM";
        case mlsl_reduction_prod:
            return "PROD";
        case mlsl_reduction_min:
            return "MIN";
        case mlsl_reduction_max:
            return "MAX";
        default:
            MLSL_ASSERT_FMT(0, "unexpected reduction %d", type);
            return "(out of range)";
    }
}

/* ***************************************************************************
 * 01234567 01234567 01234567 01234567 | 01234567 01234567 01234567 01234567  |
 *               proc_idx              |             schedule_tag             |
 *****************************************************************************/

#define MLSL_SCHED_TAG_MASK (0x00000000FFFFFFFFULL)
#define MLSL_PROC_IDX_MASK  (0xFFFFFFFF00000000ULL)
#define MLSL_PROC_IDX_SHIFT (32)

static inline uint64_t mlsl_sched_create_atl_tag(int sched_tag, size_t proc_idx)
{
    uint64_t tag = sched_tag & MLSL_SCHED_TAG_MASK;
    tag |= (proc_idx << MLSL_PROC_IDX_SHIFT);
    MLSL_LOG(DEBUG, "sched_tag %d, proc_idx %zu, tag %llu",
             sched_tag, proc_idx, (unsigned long long)tag);
    return tag;
}

mlsl_status_t mlsl_sched_dump(mlsl_sched *s, const char *name)
{
    if (!env_data.sched_dump) return mlsl_status_success;

    size_t i;
    char buf[4096] = { 0 };
    size_t buf_offset = 0;

    buf_offset += sprintf(buf + buf_offset, "\n--------------------------------\n");
    if (s) {
        buf_offset += sprintf(buf + buf_offset, "sched: %s, coll %s, %p, sz %zu, start_idx %zu, num_entries %zu, tag %d, req %p, entries %p\n",
                              name, mlsl_coll_type_to_str(s->coll_param.ctype), s, s->size, s->idx, s->num_entries, s->tag, s->req, s->entries);

        for (i = 0; i < s->num_entries; ++i) {
            mlsl_sched_entry *e = &(s->entries[i]);
            buf_offset += sprintf(buf + buf_offset, "entries[%3zu]: type %-7s, status %d, is_barrier %-5s, ",
                                  i, mlsl_sched_entry_type_to_str(e->type),
                                  e->status, (e->is_barrier ? "TRUE" : "FALSE"));

            switch (e->type) {
                case mlsl_sched_entry_send:
                    buf_offset += sprintf(buf + buf_offset, "dt %s, cnt %zu, buf %p, dst %zu, comm %p, req %p\n",
                                          mlsl_dtype_to_str(e->u.send.dtype), e->u.send.count, e->u.send.buf,
                                          e->u.send.dest, e->u.send.comm, &e->u.send.req);
                    break;
                case mlsl_sched_entry_recv:
                    buf_offset += sprintf(buf + buf_offset, "dt %s, cnt %zu, buf %p, src %zu, comm %p, req %p\n",
                                          mlsl_dtype_to_str(e->u.recv.dtype), e->u.recv.count, e->u.recv.buf,
                                          e->u.recv.src, e->u.recv.comm, &e->u.recv.req);
                    break;
                case mlsl_sched_entry_reduce:
                    buf_offset += sprintf(buf + buf_offset, "dt %s, in_buf %p, in_cnt %zu, inout_buf %p, out_cnt %p, op %s\n",
                                          mlsl_dtype_to_str(e->u.reduce.dtype),
                                          e->u.reduce.in_buf, e->u.reduce.in_count,
                                          e->u.reduce.inout_buf, e->u.reduce.out_count,
                                          mlsl_reduction_to_str(e->u.reduce.op));
                    break;
                case mlsl_sched_entry_copy:
                    buf_offset += sprintf(buf + buf_offset, "dt %s, cnt %zu, in_buf %p, out_buf %p\n",
                                          mlsl_dtype_to_str(e->u.copy.dtype), e->u.copy.count,
                                          e->u.copy.in_buf, e->u.copy.out_buf);
                    break;
                case mlsl_sched_entry_compute:
                    buf_offset += sprintf(buf + buf_offset, "dt %s, type %d, in_buf %p, in_cnt %zu, out_buf %p, out_cnt %p\n",
                                          mlsl_dtype_to_str(e->u.compute.dtype), e->u.compute.type,
                                          e->u.compute.in_buf, e->u.compute.in_count,
                                          e->u.compute.out_buf, e->u.compute.out_count);
                    break;
                case mlsl_sched_entry_sync:
                    buf_offset += sprintf(buf + buf_offset, "ptr %p, counter %d\n",
                                          e->u.sync.counter_ptr, (*e->u.sync.counter_ptr));
                    break;
                case mlsl_sched_entry_nop:
                    break;
                default:
                    MLSL_ASSERT_FMT(0, "unexpected entry_type %d", e->type);
                    break;
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
        case mlsl_sched_entry_sync:
            (*entry->u.sync.counter_ptr)++;
            break;
        case mlsl_sched_entry_nop:
            break;
        default:
            MLSL_ASSERT_FMT(0, "unexpected entry_type %d", entry->type);
            break;
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
    mlsl_status_t comp_status __attribute__ ((unused));

    MLSL_ASSERT_FMT(e->status == mlsl_sched_entry_status_not_started, "entry is already started");

    switch (e->type) {
        case mlsl_sched_entry_send:
            MLSL_LOG(DEBUG, "starting SEND entry %zu, sreq %p", idx, &e->u.send.req);
            atl_status = atl_comm_send(s->bin->comm_ctx, e->u.send.buf,
                                       e->u.send.count * mlsl_get_dtype_size(e->u.send.dtype),
                                       e->u.send.dest, mlsl_sched_create_atl_tag(s->tag, e->u.send.comm->proc_idx),
                                       &e->u.send.req);

            if (unlikely(atl_status != atl_status_success)) {
                e->status = mlsl_sched_entry_status_failed;
                MLSL_LOG(DEBUG, "SEND entry failed. atl_status: %d", atl_status);
            } else {
                e->status = mlsl_sched_entry_status_started;
            }
            break;
        case mlsl_sched_entry_recv:
            MLSL_LOG(DEBUG, "starting RECV entry %zu, rreq %p", idx, &e->u.recv.req);
            atl_status = atl_comm_recv(s->bin->comm_ctx, e->u.recv.buf,
                                       e->u.recv.count * mlsl_get_dtype_size(e->u.recv.dtype),
                                       e->u.recv.src, mlsl_sched_create_atl_tag(s->tag, e->u.recv.src),
                                       &e->u.recv.req);

            if (unlikely(atl_status != atl_status_success)) {
                e->status = mlsl_sched_entry_status_failed;
                MLSL_LOG(DEBUG, "RECV entry failed. atl_status: %d", atl_status);
            } else {
                e->status = mlsl_sched_entry_status_started;
            }
            break;
        case mlsl_sched_entry_reduce:
            MLSL_LOG(DEBUG, "starting REDUCE entry %zu", idx);
            comp_status = mlsl_comp_reduce(e->u.reduce.in_buf, e->u.reduce.in_count,
                                           e->u.reduce.inout_buf, e->u.reduce.out_count,
                                           e->u.reduce.dtype, e->u.reduce.op);
            MLSL_ASSERT(comp_status == mlsl_status_success);
            // dtype is not builtin - release ref
            // op is not builtin - release ref
            e->status = mlsl_sched_entry_status_complete;
            MLSL_LOG(DEBUG, "completed REDUCE entry %zu", idx);
            break;
        case mlsl_sched_entry_copy:
            MLSL_LOG(DEBUG, "starting COPY entry %zu", idx);
            comp_status = mlsl_comp_copy(e->u.copy.in_buf, e->u.copy.out_buf, e->u.copy.count, e->u.copy.dtype);
            MLSL_ASSERT(comp_status == mlsl_status_success);
            // indtype is not builtin - release ref
            // outdtype is not builtin - release ref
            e->status = mlsl_sched_entry_status_complete;
            MLSL_LOG(DEBUG, "completed COPY entry %zu", idx);
            break;
        case mlsl_sched_entry_sync:
            if (e->u.sync.was_used == 0)
            {
                MLSL_LOG(DEBUG, "starting SYNC entry %zu", idx);
                int prev_counter __attribute__ ((unused));
                prev_counter = __atomic_fetch_sub(e->u.sync.counter_ptr, 1, __ATOMIC_RELEASE);
                MLSL_ASSERT(prev_counter >= 0);
                e->u.sync.was_used = 1;
            }

            if (e->u.sync.was_used == 1)
            {
                int counter = __atomic_load_n(e->u.sync.counter_ptr, __ATOMIC_ACQUIRE);
                if (counter == 0)
                {
                    MLSL_LOG(DEBUG, "completed SYNC entry %zu", idx);
                    e->status = mlsl_sched_entry_status_complete;
                }
                else
                {
                    MLSL_LOG(DEBUG, "waiting SYNC entry %zu, cnt %d", idx, counter);
                    _mm_pause();
                }
            }
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
            MLSL_ASSERT_FMT(0, "unexpected entry_type %d", e->type);
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
            MLSL_ASSERT(status == mlsl_status_success);
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
    s = MLSL_MALLOC(sizeof(mlsl_sched), "schedule");

    s->size = MLSL_SCHED_INITIAL_ENTRIES;
    s->idx = 0;
    s->num_entries = 0;
    s->tag = MLSL_TAG_UNDEFINED;
    s->req = NULL;
    s->next = NULL;     /* only needed for sanity checks */
    s->prev = NULL;     /* only needed for sanity checks */

    /* this mem will be freed by the progress engine when the request is completed */
    s->entries = MLSL_MALLOC(MLSL_SCHED_INITIAL_ENTRIES * sizeof(mlsl_sched_entry),
                             "schedule entries");

    memset(&(s->coll_param), 0, sizeof(mlsl_sched_coll_param));
    memset(&(s->coll_attr), 0, sizeof(mlsl_sched_coll_attr));

    s->partial_scheds = NULL;
    s->partial_sched_count = 0;
    s->persistent_memory = NULL;

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

    s->coll_param.ctype = orig->coll_param.ctype;
    s->coll_attr = orig->coll_attr;

    size_t idx;
    for (idx = 0; idx < s->num_entries; idx++)
    {
        mlsl_sched_entry_clone(&(orig->entries[idx]), &(s->entries[idx]));
    }

    *clone = s;

    MLSL_LOG(DEBUG, "orig %p, clone %p", orig, s);

    return mlsl_status_success;
}

mlsl_status_t mlsl_sched_adjust_entries(mlsl_sched *sched, size_t partition_idx, size_t partition_count)
{
    MLSL_LOG(DEBUG, "sched %p, partition_idx %zu, partition_count %zu",
             sched, partition_idx, partition_count);

    size_t idx;
    for (idx = 0; idx < sched->num_entries; idx++)
    {
        mlsl_sched_entry_adjust(&(sched->entries[idx]), partition_idx, partition_count);
    }

    return mlsl_status_success;
}

mlsl_status_t mlsl_sched_adjust_tag(mlsl_sched *sched)
{
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
        if (sched->entries[idx].type == mlsl_sched_entry_sync)
        {
            __atomic_fetch_add(sched->entries[idx].u.sync.counter_ptr, 1, __ATOMIC_ACQUIRE);
            sched->entries[idx].u.sync.was_used = 0;
        }
    }
    return mlsl_status_success;
}

mlsl_status_t mlsl_sched_sync_schedules(mlsl_sched **scheds, size_t count)
{
    size_t idx;
    mlsl_sched_entry *sync_entry = NULL, *root_sync_entry = NULL;
    for (idx = 0; idx < count; idx++)
    {
        mlsl_sched_add_sync(scheds[idx], &sync_entry);
        if (idx == 0) root_sync_entry = sync_entry;

        MLSL_ASSERTP(root_sync_entry);
        sync_entry->u.sync.counter_ptr = root_sync_entry->u.sync.counter_ptr;
    }

    return mlsl_status_success;
}

mlsl_status_t mlsl_sched_start(mlsl_sched *s, mlsl_request **req)
{
    MLSL_ASSERTP(s && req);

    /* sanity check the schedule */
    MLSL_ASSERT(s->num_entries <= s->size);
    MLSL_ASSERT(s->num_entries == 0 || s->idx < s->num_entries);
    MLSL_ASSERT(s->req != NULL);
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
        MLSL_ASSERT_FMT(s->entries, "**nomem");
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
    MLSL_ASSERT(status == mlsl_status_success);

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
    MLSL_ASSERT(status == mlsl_status_success);

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
    mlsl_sched_reduce_local *reduce = NULL;

    status = mlsl_sched_add_entry(sched, NULL, &e);
    MLSL_ASSERT(status == mlsl_status_success);

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
    MLSL_ASSERT(status == mlsl_status_success);

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

mlsl_status_t mlsl_sched_add_sync(mlsl_sched *sched, mlsl_sched_entry **sync_entry)
{
    mlsl_status_t status = mlsl_status_success;
    mlsl_sched_entry *e = NULL;
    mlsl_sched_sync *sync = NULL;

    status = mlsl_sched_add_entry(sched, NULL, &e);
    MLSL_ASSERT(status == mlsl_status_success);

    e->type = mlsl_sched_entry_sync;
    e->status = mlsl_sched_entry_status_not_started;
    e->is_barrier = 1;
    sync = &e->u.sync;

    sync->counter = 0;
    sync->counter_ptr = &sync->counter;
    sync->was_used = 0;

    *sync_entry = e;

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
    MLSL_ASSERT(status == mlsl_status_success);

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
    mlsl_request *req __attribute__ ((unused));
    size_t sched_idx = 0;

    if (processed_sched_count)
        *processed_sched_count = 0;

    MLSL_LOG(TRACE, "bin %p, elems %p, sched_count %zu",
             bin, bin->elems, sched_count);

    /* ensure communication progress */
    atl_status_t atl_status __attribute__ ((unused));
    atl_status = atl_comm_poll(bin->comm_ctx);
    MLSL_ASSERT(atl_status == atl_status_success);

    /* update schedule states */
    MLSL_DLIST_FOREACH_SAFE(bin->elems, s, tmp) {

        if (s->first_progress)
        {
            MLSL_LOG(DEBUG, "do initial mlsl_sched_continue");
            mlsl_sched_reset(s);
            status = mlsl_sched_continue(s);
            MLSL_ASSERT(status == mlsl_status_success);
            s->first_progress = 0;
        }

        for (i = s->idx; i < s->num_entries; ++i) {
            mlsl_sched_entry *e = &s->entries[i];
            int req_status;
            switch (e->type) {
                case mlsl_sched_entry_send:
                    if (e->status == mlsl_sched_entry_status_started) {
                        atl_comm_check(bin->comm_ctx, &req_status, &e->u.send.req);
                        if (req_status) {
                            MLSL_LOG(DEBUG, "completed SEND entry %zu, sreq=%p", i, &e->u.send.req);
                            e->status = mlsl_sched_entry_status_complete;
                            mlsl_comm_release_ref(e->u.send.comm);
                        }
                    }
                    break;
                case mlsl_sched_entry_recv:
                    if (e->status == mlsl_sched_entry_status_started) {
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
                MLSL_LOG(DEBUG, "completed OTHER entry %zu, shift start_idx", i);
                if (e->is_barrier) {
                    /* post/perform the next round of operations */
                    status = mlsl_sched_continue(s);
                    MLSL_ASSERT(status == mlsl_status_success);
                }
            } else if (e->is_barrier && e->status < mlsl_sched_entry_status_complete) {
                if (e->type == mlsl_sched_entry_sync)
                    status = mlsl_sched_continue(s);
                /* don't process anything after this barrier entry */
                break;
            }
        }

        if (s->idx == s->num_entries) {
            MLSL_LOG(DEBUG, "completing and dequeuing: sched %p, req %p", s, s->req);

            /* dequeue this schedule, it's complete */
            mlsl_sched_queue_remove(bin->queue, bin, s);

            req = s->req;
            s->req = NULL;

            status = mlsl_request_complete(req);
            MLSL_ASSERT(status == mlsl_status_success);

            if (processed_sched_count)
                (*processed_sched_count)++;
        }

        sched_idx++;
        if (sched_idx == sched_count) break;
    }

    return status;
}

mlsl_status_t mlsl_sched_commit(mlsl_sched *sched)
{
    int tag;
    mlsl_sched_next_tag(global_data.comm, &tag);
    sched->tag = tag;

    mlsl_request *req;
    mlsl_request_create(&req);
    req->sched = sched;
    sched->req = req;

    mlsl_parallelizer_process(global_data.parallelizer, sched, &sched->partial_scheds,
                              &sched->partial_sched_count);

    MLSL_LOG(DEBUG, "sched %p, num_entries %zu, size %zu, tag %d, req %p, part_scheds %p, part_count %zu",
             sched, sched->num_entries, sched->size, tag, req, sched->partial_scheds, sched->partial_sched_count);

    return mlsl_status_success;
}

mlsl_status_t mlsl_sched_add_persistent_memory(mlsl_sched *sched, mlsl_sched_memory_type type, void *ptr)
{
    MLSL_LOG(DEBUG, "sched %p, type %d, ptr %p", sched, type, ptr);

    mlsl_sched_memory *mem = MLSL_CALLOC(sizeof(mlsl_sched_memory), "sched_memory");
    mem->type = type;
    mem->ptr = ptr;
    MLSL_DLIST_APPEND(sched->persistent_memory, mem);
    return mlsl_status_success;
}

mlsl_status_t mlsl_sched_free_persistent_memory(mlsl_sched *sched)
{
    MLSL_LOG(DEBUG, "sched %p", sched);

    mlsl_sched_memory *mem, *tmp;
    MLSL_DLIST_FOREACH_SAFE(sched->persistent_memory, mem, tmp)
    {
        switch (mem->type)
        {
            case mlsl_sched_memory_buffer:
                MLSL_LOG(DEBUG, "free %p", mem->ptr);
                MLSL_FREE(mem->ptr);
                break;
            case mlsl_sched_memory_registered:
                // TODO: unregister memory
                break;
        }
        MLSL_FREE(mem);
    }
    return mlsl_status_success;
}

mlsl_status_t mlsl_sched_set_coll_attr(mlsl_sched *sched, const struct mlsl_coll_attr *attr)
{
    sched->coll_attr.prologue_fn = attr->prologue_fn;
    sched->coll_attr.epilogue_fn = attr->epilogue_fn;
    sched->coll_attr.reduction_fn = attr->reduction_fn;
    sched->coll_attr.priority = attr->priority;
    sched->coll_attr.synchronous = attr->synchronous;
    sched->coll_attr.to_cache = attr->to_cache;
    if (attr->match_id)
        strncpy(sched->coll_attr.match_id, attr->match_id, MLSL_MATCH_ID_MAX_LEN - 1);

    return mlsl_status_success;
}

mlsl_status_t mlsl_sched_free(mlsl_sched *sched)
{
    size_t idx;
    for (idx = 0; idx < sched->partial_sched_count; idx++)
        mlsl_sched_free(sched->partial_scheds[idx]);
    MLSL_FREE(sched->partial_scheds);

    if (sched->req)
        mlsl_request_free(sched->req);

    mlsl_sched_free_persistent_memory(sched);
    MLSL_FREE(sched->entries);
    MLSL_FREE(sched);

    return mlsl_status_success;
}

mlsl_status_t mlsl_sched_set_prologue(mlsl_sched *sched, mlsl_sched_prologue_fn_t fn)
{
    return mlsl_status_unimplemented;
}

mlsl_status_t mlsl_sched_set_epilogue(mlsl_sched *sched, mlsl_sched_epilogue_fn_t fn)
{
    return mlsl_status_unimplemented;
}

mlsl_status_t mlsl_sched_set_reduction(mlsl_sched *sched, mlsl_sched_reduction_fn_t fn)
{
    return mlsl_status_unimplemented;
}

mlsl_status_t mlsl_sched_bcast(
    void *buf,
    size_t count,
    mlsl_data_type_t dtype,
    size_t root,
    mlsl_sched **sched)
{
    mlsl_status_t status = mlsl_status_success;

    MLSL_CALL(mlsl_sched_create(sched));

    mlsl_sched_coll_param *p = &((*sched)->coll_param);
    p->ctype = mlsl_coll_bcast;
    p->buf = buf;
    p->count = count;
    p->dtype = dtype;
    p->root = root;
    p->comm = global_data.comm;

    return status;
}

mlsl_status_t mlsl_sched_reduce(
    const void *send_buf,
    void *recv_buf,
    size_t count,
    mlsl_data_type_t dtype,
    mlsl_reduction_t reduction,
    size_t root,
    mlsl_sched **sched)
{
    mlsl_status_t status = mlsl_status_success;

    MLSL_CALL(mlsl_sched_create(sched));

    mlsl_sched_coll_param *p = &((*sched)->coll_param);
    p->ctype = mlsl_coll_reduce;
    p->send_buf = send_buf;
    p->recv_buf = recv_buf;
    p->count = count;
    p->dtype = dtype;
    p->reduction = reduction;
    p->root = root;
    p->comm = global_data.comm;

    return status;
}

mlsl_status_t mlsl_sched_allreduce(
    const void *send_buf,
    void *recv_buf,
    size_t count,
    mlsl_data_type_t dtype,
    mlsl_reduction_t reduction,
    mlsl_sched **sched)
{
    mlsl_status_t status = mlsl_status_success;

    MLSL_CALL(mlsl_sched_create(sched));

    mlsl_sched_coll_param *p = &((*sched)->coll_param);
    p->ctype = mlsl_coll_allreduce;
    p->send_buf = send_buf;
    p->recv_buf = recv_buf;
    p->count = count;
    p->dtype = dtype;
    p->reduction = reduction;
    p->comm = global_data.comm;

    return status;
}

mlsl_status_t mlsl_sched_allgatherv(
    const void *send_buf,
    size_t send_count,
    void *recv_buf,
    size_t *recv_counts,
    mlsl_data_type_t dtype,
    mlsl_sched **sched)
{
    mlsl_status_t status = mlsl_status_success;

    MLSL_CALL(mlsl_sched_create(sched));

    mlsl_sched_coll_param *p = &((*sched)->coll_param);
    p->ctype = mlsl_coll_allgatherv;
    p->send_buf = send_buf;
    p->send_count = send_count;
    p->recv_buf = recv_buf;
    p->recv_counts = recv_counts;
    p->dtype = dtype;
    p->comm = global_data.comm;

    return status;
}

mlsl_status_t mlsl_sched_barrier(mlsl_sched **sched)
{
    mlsl_status_t status = mlsl_status_success;

    MLSL_CALL(mlsl_sched_create(sched));

    mlsl_sched_coll_param *p = &((*sched)->coll_param);
    p->ctype = mlsl_coll_barrier;
    p->dtype = mlsl_dtype_char;
    p->comm = global_data.comm;

    return status;
}
