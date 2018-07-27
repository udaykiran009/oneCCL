#include "global.h"
#include "log.h"
#include "sched.h"
#include "transport.h"
#include "utils.h"

#include <stdio.h>

#define MLSL_SCHED_INITIAL_ENTRIES (16)

/* holds on to all incomplete schedules on which progress should be made */
mlsl_sched_state all_schedules = { NULL };

static const char *entry_to_str(mlsl_sched_entry_type type)
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

static void sched_dump(mlsl_sched *s, FILE * fh)
{
    int i;

    fprintf(fh, "--------------------------------\n");
    fprintf(fh, "s=%p\n", s);
    if (s) {
        fprintf(fh, "s->size=%zu\n", s->size);
        fprintf(fh, "s->idx=%zu\n", s->idx);
        fprintf(fh, "s->num_entries=%zu\n", s->num_entries);
        fprintf(fh, "s->tag=%d\n", s->tag);
        fprintf(fh, "s->req=%p\n", s->req);
        fprintf(fh, "s->entries=%p\n", s->entries);
        for (i = 0; i < s->num_entries; ++i) {
            fprintf(fh, "&s->entries[%d]=%p\n", i, &s->entries[i]);
            fprintf(fh, "s->entries[%d].type=%s\n", i, entry_to_str(s->entries[i].type));
            fprintf(fh, "s->entries[%d].status=%d\n", i, s->entries[i].status);
            fprintf(fh, "s->entries[%d].is_barrier=%s\n", i,
                    (s->entries[i].is_barrier ? "TRUE" : "FALSE"));
        }
    }
    fprintf(fh, "--------------------------------\n");
    fprintf(fh, "s->next=%p\n", s->next);
    fprintf(fh, "s->prev=%p\n", s->prev);
}

mlsl_status_t mlsl_sched_next_tag(mlsl_comm *comm, int *tag)
{
    mlsl_status_t status = mlsl_status_success;
    int tag_ub = global_data.tag_ub;
    int start = MLSL_TAG_UNDEFINED;
    int end = MLSL_TAG_UNDEFINED;
    mlsl_sched *elt = NULL;

    *tag = comm->next_sched_tag;
    ++comm->next_sched_tag;

    /* Upon entry into the second half of the tag space, ensure there are no
     * outstanding schedules still using the second half of the space.  Check
     * the first half similarly on wraparound. */
    if (comm->next_sched_tag == (tag_ub / 2)) {
        start = tag_ub / 2;
        end = tag_ub;
    } else if (comm->next_sched_tag == (tag_ub)) {
        start = MLSL_TAG_FIRST;
        end = tag_ub / 2;
    }
    if (start != MLSL_TAG_UNDEFINED) {
        MLSL_DLIST_FOREACH(all_schedules.head, elt) {
            if (elt->tag >= start && elt->tag < end) {
                MLSL_ASSERTP_FMT(0, "**toomanynbc");
            }
        }
    }

    /* wrap the tag values around to the start */
    if (comm->next_sched_tag == tag_ub) {
        comm->next_sched_tag = MLSL_TAG_FIRST;
    }

    return status;
}

/* initiates the schedule entry "e" in the NBC described by "s", where
 * "e" is at "idx" in "s".  This means posting nonblocking sends/recvs,
 * performing reductions, calling callbacks, etc. */
mlsl_status_t mlsl_sched_start_entry(mlsl_sched *s, size_t idx, mlsl_sched_entry *e)
{
    mlsl_status_t status = mlsl_status_success;
    mlsl_status_t comm_status = mlsl_status_success, comp_status = mlsl_status_success;
    mlsl_comm *comm;

    MLSL_ASSERT_FMT(e->status == mlsl_sched_entry_status_not_started, "entry is already started");

    switch (e->type) {
        case mlsl_sched_entry_send:
            comm = e->u.send.comm;
            MLSL_LOG(DEBUG, "starting SEND entry %zu", idx);
                comm_status = mlsl_transport_send();
                // mlsl_transport_send(e->u.send.buf, e->u.send.count, e->u.send.dtype,
                //                     e->u.send.dest, s->tag, comm, &e->u.send.req);

            /* Check if the error is actually fatal to the NBC or we can continue. */
            if (unlikely(comm_status)) {
                e->status = mlsl_sched_entry_status_failed;
                MLSL_LOG(DEBUG, "Sched SEND failed. comm_status: %d", comm_status);
            } else {
                e->status = mlsl_sched_entry_status_started;
            }
            break;
        case mlsl_sched_entry_recv:
            MLSL_LOG(DEBUG, "starting RECV entry %zu", idx);
            comm = e->u.recv.comm;
            comm_status = mlsl_transport_recv();
            // mlsl_transport_recv(e->u.recv.buf, e->u.recv.count, e->u.recv.dtype,
            //                     e->u.recv.src, s->tag, comm, &e->u.recv.req);

            /* Check if the error is actually fatal to the NBC or we can continue. */
            if (unlikely(comm_status)) {
                e->status = mlsl_sched_entry_status_failed; // TODO
                MLSL_LOG(DEBUG, "Sched RECV failed. comm_status: %d", comm_status);
            } else {
                e->status = mlsl_sched_entry_status_started;
            }
            break;
        case mlsl_sched_entry_reduce:
            MLSL_LOG(DEBUG, "starting REDUCE entry %zu", idx);

            // TODO: create module with computation kernels
            // comp_status = 
            //     mlsl_comp_reduce(e->u.reduce.in_buf, e->u.reduce.inout_buf, e->u.reduce.count,
            //                      e->u.reduce.dtype, e->u.reduce.op);
            MLSL_ASSERTP(comp_status == mlsl_status_success);
            // dtype is not builtin - release ref
            // op is not builtin - release ref
            e->status = mlsl_sched_entry_status_complete;
            break;
        case mlsl_sched_entry_copy:
            MLSL_LOG(DEBUG, "starting COPY entry %zu", idx);
            comp_status = mlsl_status_success;
            memcpy(e->u.copy.out_buf, e->u.copy.in_buf, e->u.copy.count * mlsl_get_dtype_size(e->u.copy.dtype));
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
                status = e->u.compute.u.fn_1i1o(e->u.compute.in_buf, e->u.compute.in_count, e->u.compute.dtype,
                                                e->u.compute.out_buf, e->u.compute.out_count);
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
    s = MLSL_MALLOC(sizeof(mlsl_sched), CACHELINE_SIZE);
    MLSL_ASSERTP_FMT(s, "schedule object");

    s->size = MLSL_SCHED_INITIAL_ENTRIES;
    s->idx = 0;
    s->num_entries = 0;
    s->tag = MLSL_TAG_UNDEFINED;
    s->req = NULL;
    s->entries = NULL;
    s->next = NULL;     /* only needed for sanity checks */
    s->prev = NULL;     /* only needed for sanity checks */

    /* this mem will be freed by the progress engine when the request is completed */
    s->entries = MLSL_MALLOC(MLSL_SCHED_INITIAL_ENTRIES * sizeof(mlsl_sched_entry), CACHELINE_SIZE);
    MLSL_ASSERTP_FMT(s, "schedule entries vector");

    *sp = s;
    return status;
}

/* clones orig and returns a handle to the new schedule in (*cloned) */
mlsl_status_t mlsl_sched_clone(mlsl_sched *orig, mlsl_sched **cloned)
{
    return mlsl_status_unimplemented;
}

/* sets (*sp) to NULL and gives you back a request pointer in (*req).
 * The caller is giving up ownership of the opaque schedule object. */
mlsl_status_t mlsl_sched_start(mlsl_sched *s, mlsl_request **req)
{
    return mlsl_status_unimplemented;

    // TODO: create comm and tag on commit phase
    mlsl_comm *comm;
    int tag;

    mlsl_status_t status = mlsl_status_success;

    mlsl_request *r;
    *req = NULL;

    /* sanity check the schedule */
    MLSL_ASSERT(s->num_entries <= s->size);
    MLSL_ASSERT(s->num_entries == 0 || s->idx < s->num_entries);
    MLSL_ASSERT(s->req == NULL);
    MLSL_ASSERT(s->next == NULL);
    MLSL_ASSERT(s->prev == NULL);
    MLSL_ASSERT(s->entries != NULL);

    /* now create and populate the request */
    mlsl_request_create(&r);
    MLSL_ASSERT_FMT(r, "**nomem");

    /* FIXME is this right when comm/datatype GC is used? */
    mlsl_comm_add_ref(comm);
    r->comm = comm;

    /* req refcount is currently 1, for the user's request.  Increment for the
     * schedule's reference */
    mlsl_request_add_ref(r);
    s->req = r;
    *req = r;
    /* cc is 1, which is fine b/c we only use it as a signal, rather than
     * incr/decr on every constituent operation */
    s->tag = tag;

    /* Now kick off any initial operations.  Do this before we tell the progress
     * engine about this req+sched, otherwise we have more MT issues to worry
     * about.  Skipping this step will increase latency. */

    // TODO: this should be executed in worker threads
    status = mlsl_sched_continue(s);
    MLSL_ASSERTP(status == mlsl_status_success);

    /* finally, enqueue in the list of all pending schedules so that the
     * progress engine can make progress on it */

    MLSL_DLIST_APPEND(all_schedules.head, s);

    // TODO: copy, adjust and propagate scheds to workers

    MLSL_LOG(DEBUG, "started schedule s=%p", s);
    if (env_data.dump_sched)
        sched_dump(s, stderr);

    return status;
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
                                  2 * s->size * sizeof(mlsl_sched_entry), CACHELINE_SIZE);
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
    e->u.send.req = NULL;      /* will be populated by _start_entry */
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
    e->u.recv.req = NULL;      /* will be populated by _start_entry */
    e->u.recv.comm = global_data.comm;

    mlsl_comm_add_ref(e->u.recv.comm);

    return status;
}

mlsl_status_t mlsl_sched_add_reduce(mlsl_sched *sched, const void *in_buf, void *inout_buf,
                                    size_t count, mlsl_data_type_t dtype, mlsl_reduction_t op)
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
    reduce->inout_buf = inout_buf;
    reduce->count = count;
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
                                          const void* in_buf, size_t in_count, mlsl_data_type_t dtype,
                                          void * out_buf, size_t *out_count)
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
    compute->dtype = dtype;
    compute->out_buf = out_buf;
    compute->out_count = out_count;

    // TODO: add send_defer and callback primitives for resource cleanup

    return status;
}


/* returns TRUE in (*made_progress) if any of the outstanding schedules in state completed */
static mlsl_status_t mlsl_sched_progress_state(mlsl_sched_state *state, int *made_progress)
{
    mlsl_status_t status = mlsl_status_success;
    size_t i;
    mlsl_sched *s;
    mlsl_sched *tmp;

    if (made_progress)
        *made_progress = 0;

    MLSL_DLIST_FOREACH_SAFE(state->head, s, tmp) {
        if (env_data.dump_sched)
            sched_dump(s, stderr);

        for (i = s->idx; i < s->num_entries; ++i) {
            mlsl_sched_entry *e = &s->entries[i];

            switch (e->type) {
                case mlsl_sched_entry_send:
                    if (e->u.send.req != NULL && mlsl_request_is_complete(e->u.send.req)) {
                        MLSL_LOG(DEBUG, "completed SEND entry %zu, sreq=%p", i, e->u.send.req);
                        e->status = mlsl_sched_entry_status_complete;
                        mlsl_request_free(e->u.send.req);
                        e->u.send.req = NULL;
                        mlsl_comm_release_ref(e->u.send.comm);
                    }
                    break;
                case mlsl_sched_entry_recv:
                    if (e->u.recv.req != NULL && mlsl_request_is_complete(e->u.recv.req)) {
                        MLSL_LOG(DEBUG, "completed RECV entry %zu, rreq=%p", i, e->u.recv.req);
                        e->status = mlsl_sched_entry_status_complete;
                        mlsl_request_free(e->u.recv.req);
                        e->u.recv.req = NULL;
                        mlsl_comm_release_ref(e->u.recv.comm);
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
            MLSL_LOG(DEBUG, "completing and dequeuing s=%p r=%p", s, s->req);

            /* dequeue this schedule from the state, it's complete */
            MLSL_DLIST_DELETE(state->head, s);

            status = mlsl_request_complete(s->req);
            MLSL_ASSERTP(status == mlsl_status_success);

            s->req = NULL;
            MLSL_FREE(s->entries);
            MLSL_FREE(s);

            if (made_progress)
                *made_progress = 1;
        }
    }

    return status;
}

/* returns TRUE in (*made_progress) if any of the outstanding schedules completed */
mlsl_status_t mlsl_sched_progress(int *made_progress)
{
    mlsl_status_t status = mlsl_status_success;

    status = mlsl_sched_progress_state(&all_schedules, made_progress);

    // TODO: check sched_queue and maybe sleep if queue is empty

    return status;
}

mlsl_status_t mlsl_sched_commit(mlsl_sched *sched)
{
    if (env_data.dump_sched)
        sched_dump(sched, stderr);

    return mlsl_status_success;
}

mlsl_status_t mlsl_sched_free(mlsl_sched *sched)
{
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
    mlsl_sched_t *sched)
{
    return mlsl_status_unimplemented;
}
