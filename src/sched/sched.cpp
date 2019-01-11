#include "sched/sched.hpp"
#include "sched/sched_queue.hpp"
#include "comp/comp.hpp"
#include "common/global/global.hpp"
#include "common/log/log.hpp"
#include "common/utils/utils.hpp"
#include "common/comm/comm.hpp"

#include <stdio.h>

#define MLSL_SCHED_INITIAL_ENTRIES (16)

size_t mlsl_sched_get_priority(mlsl_sched *sched)
{
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
        case mlsl_sched_entry_recv_reduce:
            return "RECV_REDUCE";
        case mlsl_sched_entry_copy:
            return "COPY";
        case mlsl_sched_entry_sync:
            return "SYNC";
        case mlsl_sched_entry_collective:
            return "COLLECTIVE";
        case mlsl_sched_entry_prologue:
            return "PROLOGUE";
        case mlsl_sched_entry_epilogue:
            return "EPILOGUE";
        case mlsl_sched_entry_update_postponed_fields:
            return "UPDATE_FIELDS";
        case mlsl_sched_entry_nop:
            return "NOP";
        default:
            return "UNKNOWN";
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
        case mlsl_reduction_custom:
            return "CUSTOM";
        default:
            return "UNKNOWN";
    }
}

mlsl_status_t mlsl_sched_dump(mlsl_sched *s, const char *name)
{
    if (!env_data.sched_dump) return mlsl_status_success;

    size_t i;
    char buf[32768] = { 0 };
    size_t buf_offset = 0;

    buf_offset += sprintf(buf + buf_offset, "\n--------------------------------\n");
    if (s) {
        buf_offset += sprintf(buf + buf_offset, "sched: %s, coll %s, %p, sz %zu, start_idx %zu, "
                              "num_entries %zu, number %u, req %p, entries %p\n",
                              name, mlsl_coll_type_to_str(s->coll_param.ctype), s, s->size, s->idx,
                              s->num_entries, s->sched_id, s->req, s->entries);

        for (i = 0; i < s->num_entries; ++i) {
            mlsl_sched_entry *e = &(s->entries[i]);
            buf_offset += sprintf(buf + buf_offset, "entries[%3zu]: type %-7s, status %d, is_barrier %-5s, ",
                                  i, mlsl_sched_entry_type_to_str(e->type),
                                  e->status, (e->is_barrier ? "TRUE" : "FALSE"));

            switch (e->type) {
                case mlsl_sched_entry_send:
                    buf_offset += sprintf(buf + buf_offset, "dt %s, cnt %zu, buf %p, dst %zu, comm %p, req %p\n",
                                          mlsl_datatype_get_name(e->u.send.dtype), e->u.send.count, e->u.send.buf,
                                          e->u.send.dest, e->u.send.comm, &e->u.send.req);
                    break;
                case mlsl_sched_entry_recv:
                    buf_offset += sprintf(buf + buf_offset, "dt %s, cnt %zu, buf %p, src %zu, comm %p, req %p\n",
                                          mlsl_datatype_get_name(e->u.recv.dtype), e->u.recv.count, e->u.recv.buf,
                                          e->u.recv.src, e->u.recv.comm, &e->u.recv.req);
                    break;
                case mlsl_sched_entry_reduce:
                    buf_offset += sprintf(buf + buf_offset, "dt %s, in_buf %p, in_cnt %zu, inout_buf %p, out_cnt %p, op %s, red_fn %p\n",
                                          mlsl_datatype_get_name(e->u.reduce.dtype),
                                          e->u.reduce.in_buf, e->u.reduce.in_count,
                                          e->u.reduce.inout_buf, e->u.reduce.out_count,
                                          mlsl_reduction_to_str(e->u.reduce.op),
                                          e->u.reduce.reduction_fn);
                    break;
                case mlsl_sched_entry_recv_reduce:
                    buf_offset += sprintf(buf + buf_offset, "dt %s, inout_buf %p, in_cnt %zu, out_cnt %p, op %s, red_fn %p, "
                                          "src %zu, comm_buf %p, comm %p, req %p\n",
                                          mlsl_datatype_get_name(e->u.recv_reduce.dtype),
                                          e->u.recv_reduce.inout_buf, e->u.recv_reduce.in_count,
                                          e->u.recv_reduce.out_count,
                                          mlsl_reduction_to_str(e->u.recv_reduce.op),
                                          e->u.recv_reduce.reduction_fn,
                                          e->u.recv_reduce.src, e->u.recv_reduce.comm_buf,
                                          e->u.recv_reduce.comm, &e->u.recv_reduce.req);
                    break;
                case mlsl_sched_entry_copy:
                    buf_offset += sprintf(buf + buf_offset, "dt %s, cnt %zu, in_buf %p, out_buf %p\n",
                                          mlsl_datatype_get_name(e->u.copy.dtype), e->u.copy.count,
                                          e->u.copy.in_buf, e->u.copy.out_buf);
                    break;
                case mlsl_sched_entry_sync:
                    buf_offset += sprintf(buf + buf_offset, "root_sched %p, counter %d\n",
                                          e->u.sync.root_sched, e->u.sync.root_sched->entries[e->u.sync.entry_idx].u.sync.counter);
                    break;
                case mlsl_sched_entry_collective:
                    buf_offset += sprintf(buf + buf_offset, "dt %s, coll_type %s, send_buf %p, recv_buf %p, count %zu, op %s, "
                                          "comm %p, req %p\n",
                                          mlsl_datatype_get_name(e->u.coll.dtype), mlsl_coll_type_to_str(e->u.coll.ctype),
                                          e->u.coll.send_buf, e->u.coll.recv_buf, e->u.coll.count, mlsl_reduction_to_str(e->u.coll.op),
                                          e->u.coll.comm, e->u.coll.req);
                    break;
                case mlsl_sched_entry_prologue:
                    buf_offset += sprintf(buf + buf_offset, "in_dt %s, in_cnt %zu, in_buf %p, out_dt %s, out_cnt %p, out_buf %p, fn %p\n",
                                          mlsl_datatype_get_name(e->u.prologue.in_dtype), e->u.prologue.in_count, e->u.prologue.in_buf,
                                          mlsl_datatype_get_name(e->u.prologue.out_dtype), e->u.prologue.out_count, e->u.prologue.out_buf,
                                          e->u.prologue.fn);
                    break;
                case mlsl_sched_entry_epilogue:
                    buf_offset += sprintf(buf + buf_offset, "in_dt %s, in_cnt %zu, in_buf %p, out_dt %s, out_cnt %p, out_buf %p, fn %p, exp_out_count %zu\n",
                                          mlsl_datatype_get_name(e->u.epilogue.in_dtype), e->u.epilogue.in_count, e->u.epilogue.in_buf,
                                          mlsl_datatype_get_name(e->u.epilogue.out_dtype), e->u.epilogue.out_count, e->u.epilogue.out_buf,
                                          e->u.epilogue.fn, e->u.epilogue.expected_out_count);
                    break;
                case mlsl_sched_entry_update_postponed_fields:
                    buf_offset += sprintf(buf + buf_offset, "part_idx %zu, part_count %zu\n",
                                          e->u.update_fields.part_idx,
                                          e->u.update_fields.part_count);
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
        out_offset = part_idx * out_count * mlsl_datatype_get_size(dtype);                   \
        if (part_idx == (part_count - 1)) out_count += count % part_count;                   \
    } while (0)

#define MLSL_ADJUST_PTR(ptr, offset) ptr = (char*)ptr + offset

static void mlsl_sched_entry_adjust(mlsl_sched_entry *entry, size_t partition_idx, size_t partition_count)
{
    MLSL_ASSERTP(0);

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
        case mlsl_sched_entry_recv_reduce:
            MLSL_GET_COUNT_AND_OFFSET(entry->u.recv_reduce.in_count, entry->u.recv_reduce.dtype,
                                      partition_idx, partition_count,
                                      adjust_count, adjust_offset);
            entry->u.recv_reduce.in_count = adjust_count;
            MLSL_ADJUST_PTR(entry->u.recv_reduce.inout_buf, adjust_offset);
            MLSL_ADJUST_PTR(entry->u.recv_reduce.comm_buf, adjust_offset);
            break;
        case mlsl_sched_entry_copy:
            MLSL_GET_COUNT_AND_OFFSET(entry->u.copy.count, entry->u.copy.dtype,
                                      partition_idx, partition_count,
                                      adjust_count, adjust_offset);
            entry->u.copy.count = adjust_count;
            MLSL_ADJUST_PTR(entry->u.copy.in_buf, adjust_offset);
            MLSL_ADJUST_PTR(entry->u.copy.out_buf, adjust_offset);
            break;
        case mlsl_sched_entry_sync:
            entry->u.sync.root_sched->entries[entry->u.sync.entry_idx].u.sync.counter++;
            break;
        case mlsl_sched_entry_nop:
            break;
        default:
            MLSL_ASSERTP_FMT(0, "unexpected entry_type %d", entry->type);
            break;
    }
    return;
}

/* initiates the schedule entry "e" in the NBC described by "s", where
 * "e" is at "idx" in "s".  This means posting nonblocking sends/recvs,
 * performing reductions, calling callbacks, etc. */
mlsl_status_t mlsl_sched_start_entry(mlsl_sched *s, size_t idx, mlsl_sched_entry *e)
{
    mlsl_status_t status = mlsl_status_success;
    atl_status_t atl_status = atl_status_success;
    mlsl_status_t comp_status __attribute__ ((unused));
    mlsl_datatype_internal_t elem_dtype;
    mlsl_datatype_internal* elem_dtype_nonconst;
    size_t elem_count;
    const void* elem_buf, *elem_sbuf;
    void *elem_rbuf;
    mlsl_sched* coll_sched = NULL;
    mlsl_request *coll_req = NULL;
    int* counter_ptr;
    int counter, prev_counter;
    size_t part_idx, part_count, orig_count;

    MLSL_ASSERTP_FMT(e->status == mlsl_sched_entry_status_not_started ||
      (e->status == mlsl_sched_entry_status_started &&
        (e->type == mlsl_sched_entry_recv_reduce ||
         e->type == mlsl_sched_entry_sync ||
         e->type == mlsl_sched_entry_collective)),
      "entry is already started");

    switch (e->type) {
        case mlsl_sched_entry_send:
            MLSL_LOG(DEBUG, "starting SEND entry %zu, dest %zu, sreq %p", idx, e->u.send.dest, &e->u.send.req);
            atl_status = atl_comm_send(s->bin->comm_ctx, e->u.send.buf,
                                       e->u.send.count * mlsl_datatype_get_size(e->u.send.dtype),
                                       e->u.send.dest,
                                       mlsl_create_atl_tag(s->coll_param.comm->comm_id, s->sched_id, e->u.send.comm->rank),
                                       &e->u.send.req);

            if (unlikely(atl_status != atl_status_success)) {
                e->status = mlsl_sched_entry_status_failed;
                MLSL_LOG(DEBUG, "SEND entry failed. atl_status: %d", atl_status);
            } else {
                e->status = mlsl_sched_entry_status_started;
            }
            break;
        case mlsl_sched_entry_recv:
            MLSL_LOG(DEBUG, "starting RECV entry %zu, src %zu, rreq %p", idx, e->u.recv.src, &e->u.recv.req);
            atl_status = atl_comm_recv(s->bin->comm_ctx, e->u.recv.buf,
                                       e->u.recv.count * mlsl_datatype_get_size(e->u.recv.dtype),
                                       e->u.recv.src,
                                       mlsl_create_atl_tag(s->coll_param.comm->comm_id, s->sched_id, e->u.recv.src),
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
                                           e->u.reduce.dtype, e->u.reduce.op,
                                           e->u.reduce.reduction_fn);
            MLSL_ASSERT(comp_status == mlsl_status_success);
            e->status = mlsl_sched_entry_status_complete;
            MLSL_LOG(DEBUG, "completed REDUCE entry %zu", idx);
            break;
        case mlsl_sched_entry_recv_reduce:
            MLSL_LOG(DEBUG, "starting RECV_REDUCE entry %zu, rreq %p", idx, &e->u.recv_reduce.req);
            if (e->status != mlsl_sched_entry_status_started)
            {
                MLSL_LOG(DEBUG, "starting RECV in RECV_REDUCE entry %zu, rreq %p", idx, &e->u.recv_reduce.req);
                atl_status = atl_comm_recv(s->bin->comm_ctx, e->u.recv_reduce.comm_buf,
                                           e->u.recv_reduce.in_count * mlsl_datatype_get_size(e->u.recv_reduce.dtype),
                                           e->u.recv_reduce.src,
                                           mlsl_create_atl_tag(s->coll_param.comm->comm_id, s->sched_id, e->u.recv_reduce.src),
                                           &e->u.recv_reduce.req);

                if (unlikely(atl_status != atl_status_success)) {
                    e->status = mlsl_sched_entry_status_failed;
                    MLSL_LOG(DEBUG, "RECV entry failed. atl_status: %d", atl_status);
                } else {
                    e->status = mlsl_sched_entry_status_started;
                }
            }
            else
            {
                MLSL_LOG(DEBUG, "starting REDUCE in RECV_REDUCE entry %zu", idx);
                comp_status = mlsl_comp_reduce(e->u.recv_reduce.comm_buf, e->u.recv_reduce.in_count,
                                               e->u.recv_reduce.inout_buf, e->u.recv_reduce.out_count,
                                               e->u.recv_reduce.dtype, e->u.recv_reduce.op,
                                               e->u.recv_reduce.reduction_fn);
                MLSL_ASSERT(comp_status == mlsl_status_success);
                e->status = mlsl_sched_entry_status_complete;
                MLSL_LOG(DEBUG, "completed REDUCE in RECV_REDUCE entry %zu", idx);
            }
            break;
        case mlsl_sched_entry_copy:
            MLSL_LOG(DEBUG, "starting COPY entry %zu", idx);
            elem_buf = (e->u.copy.in_buf == MLSL_POSTPONED_ADDR) ?
                        s->postponed_fields.buf :
                        e->u.copy.in_buf;
            elem_count = (e->u.copy.count == MLSL_POSTPONED_COUNT) ?
                          s->postponed_fields.count :
                          e->u.copy.count;
            elem_dtype = (e->u.copy.dtype == MLSL_POSTPONED_DTYPE) ?
                          &(s->postponed_fields.dtype) :
                          e->u.copy.dtype;
            comp_status = mlsl_comp_copy(elem_buf, e->u.copy.out_buf, elem_count, elem_dtype);
            MLSL_ASSERT(comp_status == mlsl_status_success);
            e->status = mlsl_sched_entry_status_complete;
            MLSL_LOG(DEBUG, "completed COPY entry %zu", idx);
            break;
        case mlsl_sched_entry_sync:
            counter_ptr = &e->u.sync.root_sched->entries[e->u.sync.entry_idx].u.sync.counter;
            if (e->status == mlsl_sched_entry_status_not_started)
            {
                MLSL_LOG(DEBUG, "starting SYNC entry %zu", idx);
                prev_counter = __atomic_fetch_sub(counter_ptr, 1, __ATOMIC_RELEASE);
                MLSL_ASSERT(prev_counter >= 0);
                e->status = mlsl_sched_entry_status_started;
            }

            if (e->status == mlsl_sched_entry_status_started)
            {
                counter = __atomic_load_n(counter_ptr, __ATOMIC_ACQUIRE);
                if (counter == 0)
                {
                    MLSL_LOG(DEBUG, "completed SYNC entry %zu", idx);
                    e->status = mlsl_sched_entry_status_complete;
                }
                else
                {
                    MLSL_LOG(TRACE, "waiting SYNC entry %zu, cnt %d", idx, counter);
                    _mm_pause();
                }
            }
            break;
        case mlsl_sched_entry_collective:
            if (e->status == mlsl_sched_entry_status_started)
            {
                MLSL_ASSERTP(e->u.coll.req);
                if (mlsl_request_is_complete(e->u.coll.req))
                {
                    MLSL_LOG(DEBUG, "COLLECTIVE entry %zu, completed coll_sched", idx);
                    mlsl_sched_free(e->u.coll.req->sched);
                    e->status = mlsl_sched_entry_status_complete;
                }
                break;
            }
            switch (e->u.coll.ctype)
            {
                case mlsl_coll_barrier:
                case mlsl_coll_bcast:
                case mlsl_coll_reduce:
                    break;
                case mlsl_coll_allreduce:
                    elem_sbuf = (e->u.coll.send_buf == MLSL_POSTPONED_ADDR) ?
                                 s->postponed_fields.buf :
                                 e->u.coll.send_buf;
                    elem_rbuf = (e->u.coll.recv_buf == MLSL_POSTPONED_ADDR) ?
                                 s->postponed_fields.buf :
                                 e->u.coll.recv_buf;
                    elem_count = (e->u.coll.count == MLSL_POSTPONED_COUNT) ?
                                  s->postponed_fields.count :
                                  e->u.coll.count;
                    elem_dtype = (e->u.coll.dtype == MLSL_POSTPONED_DTYPE) ?
                                  &(s->postponed_fields.dtype) :
                                  e->u.coll.dtype;
                    MLSL_CALL(mlsl_sched_allreduce(elem_sbuf, elem_rbuf, elem_count,
                                                   elem_dtype, e->u.coll.op,
                                                   s->coll_param.comm, &coll_sched));
                    coll_sched->coll_attr.reduction_fn = s->coll_attr.reduction_fn;
                    MLSL_CALL(mlsl_coll_build_allreduce(coll_sched,
                                                        coll_sched->coll_param.send_buf,
                                                        coll_sched->coll_param.recv_buf,
                                                        coll_sched->coll_param.count,
                                                        coll_sched->coll_param.dtype,
                                                        coll_sched->coll_param.reduction));
                    break;
                case mlsl_coll_allgatherv:
                case mlsl_coll_custom:
                default:
                    /* only allreduce for now */
                    MLSL_ASSERTP(0);
                    break;
            }
            if (coll_sched)
            {
                MLSL_LOG(DEBUG, "starting COLLECTIVE entry %zu", idx);
                coll_sched->sched_id = s->sched_id;
                mlsl_request_create(&coll_req);
                coll_req->completion_counter = 1;
                coll_req->sched = coll_sched;
                coll_sched->req = coll_req;
                e->u.coll.req = coll_req;
                coll_sched->sched_id = s->sched_id;
                mlsl_sched_reset(coll_sched);
                mlsl_sched_dump(coll_sched, "coll_sched");    
                mlsl_sched_queue_add(s->bin->queue, coll_sched, mlsl_sched_get_priority(s));
                MLSL_LOG(DEBUG, "COLLECTIVE entry %zu: queue %p, sched %p, req %p",
                         idx, s->bin->queue, coll_sched, coll_req);
                // TODO: insert into per-worker sched cache
            }
            e->status = mlsl_sched_entry_status_started;
            break;
        case mlsl_sched_entry_prologue:
            MLSL_LOG(DEBUG, "starting PROLOGUE entry %zu", idx);
            elem_dtype_nonconst = e->u.prologue.out_dtype;
            e->u.prologue.fn(e->u.prologue.in_buf, e->u.prologue.in_count, e->u.prologue.in_dtype->type,
                             e->u.prologue.out_buf, e->u.prologue.out_count, &(elem_dtype_nonconst->type),
                             &(elem_dtype_nonconst->size));
            e->status = mlsl_sched_entry_status_complete;
            break;
        case mlsl_sched_entry_epilogue:
            MLSL_LOG(DEBUG, "starting EPILOGUE entry %zu", idx);
            elem_buf = (e->u.epilogue.in_buf == MLSL_POSTPONED_ADDR) ?
                        s->postponed_fields.buf :
                        e->u.epilogue.in_buf;
            elem_count = (e->u.epilogue.in_count == MLSL_POSTPONED_COUNT) ?
                          s->postponed_fields.count :
                          e->u.epilogue.in_count;
            elem_dtype = (e->u.epilogue.in_dtype == MLSL_POSTPONED_DTYPE) ?
                          &(s->postponed_fields.dtype) :
                           e->u.epilogue.in_dtype;
            e->u.epilogue.fn(elem_buf, elem_count, elem_dtype->type,
                             e->u.epilogue.out_buf, e->u.epilogue.out_count, e->u.epilogue.out_dtype->type);
            MLSL_ASSERTP(e->u.epilogue.expected_out_count == (*e->u.epilogue.out_count));
            e->status = mlsl_sched_entry_status_complete;
            break;
        case mlsl_sched_entry_update_postponed_fields:
            elem_dtype = &s->postponed_fields.dtype;
            part_idx = e->u.update_fields.part_idx;
            part_count = e->u.update_fields.part_count;
            orig_count = s->postponed_fields.count;
            s->postponed_fields.count = orig_count / part_count;
            s->postponed_fields.buf = (char*)s->postponed_fields.buf + part_idx * s->postponed_fields.count * mlsl_datatype_get_size(elem_dtype);
            if (part_idx == (part_count - 1)) s->postponed_fields.count += orig_count % part_count;
            MLSL_LOG(DEBUG, "UPDATE_FIELDS: entry %zu, part_idx %zu, part_count %zu, dtype %s, count %zu, buf %p",
                     idx, part_idx, part_count, mlsl_datatype_get_name(elem_dtype), s->postponed_fields.count, s->postponed_fields.buf);
            e->status = mlsl_sched_entry_status_complete;
            break;
        case mlsl_sched_entry_nop:
            MLSL_LOG(DEBUG, "starting NOP entry %zu", idx);
            /* nothing to be done */
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
    s = static_cast<mlsl_sched*>(MLSL_MALLOC(sizeof(mlsl_sched), "schedule"));

    s->size = MLSL_SCHED_INITIAL_ENTRIES;
    s->idx = 0;
    s->num_entries = 0;
    s->sched_id = 0;
    s->req = NULL;
    s->next = NULL;     /* only needed for sanity checks */
    s->prev = NULL;     /* only needed for sanity checks */

    /* this mem will be freed by the progress engine when the request is completed */
    s->entries = static_cast<mlsl_sched_entry*>(MLSL_MALLOC(MLSL_SCHED_INITIAL_ENTRIES * sizeof(mlsl_sched_entry),
                             "schedule entries"));

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
    mlsl_sched *s = static_cast<mlsl_sched*>(MLSL_CALLOC(sizeof(mlsl_sched), "schedule"));

    s->size = orig->size;
    s->idx = 0;
    s->num_entries = orig->num_entries;
    s->sched_id = orig->sched_id;
    s->req = orig->req; // cloned scheds point to origin req for completion notification
    s->entries = NULL;
    s->next = NULL;
    s->prev = NULL;
    s->entries = static_cast<mlsl_sched_entry*>(MLSL_CALLOC(s->size * sizeof(mlsl_sched_entry),
                                                            "schedule entries"));

    s->coll_param.ctype = orig->coll_param.ctype;
    s->coll_param.comm = orig->coll_param.comm;
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
    sched->sched_id = mlsl_comm_get_sched_id(sched->coll_param.comm);

    return mlsl_status_success;
}

mlsl_status_t mlsl_sched_reset(mlsl_sched *sched)
{
    size_t idx;
    sched->idx = 0;
    sched->first_progress = 1;
    for (idx = 0; idx < sched->num_entries; idx++)
    {
        sched->entries[idx].status = mlsl_sched_entry_status_not_started;
        if (sched->entries[idx].type == mlsl_sched_entry_sync)
        {
            if (sched->entries[idx].u.sync.root_sched == sched)
            {
                // reset counter for the root sched only
                sched->entries[idx].u.sync.counter = sched->entries[idx].u.sync.initial_counter;
            }
        }
    }
    return mlsl_status_success;
}

mlsl_status_t mlsl_sched_sync_schedules(mlsl_sched **scheds, size_t count)
{
    size_t idx;
    mlsl_sched_entry *sync_entry = NULL, *root_sync_entry = NULL;
    int root_sync_entry_idx = -1;
    for (idx = 0; idx < count; idx++)
    {
        int sync_entry_idx;
        mlsl_sched_add_sync(scheds[idx], &sync_entry, &sync_entry_idx);
        if (idx == 0)
        {
            root_sync_entry = sync_entry;
            root_sync_entry_idx = sync_entry_idx;
        }
        // save pointer to the root
        sync_entry->u.sync.root_sched = scheds[0];
        MLSL_ASSERTP(root_sync_entry_idx >= 0);
        sync_entry->u.sync.entry_idx = root_sync_entry_idx;
    }

    MLSL_ASSERTP(root_sync_entry);

    root_sync_entry->u.sync.initial_counter = count;
    root_sync_entry->u.sync.counter = count;

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
    MLSL_ASSERT(s->coll_param.comm);

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
        s->entries = static_cast<mlsl_sched_entry*>(MLSL_REALLOC(s->entries,
                                  s->size * sizeof(mlsl_sched_entry),
                                  2 * s->size * sizeof(mlsl_sched_entry), CACHELINE_SIZE,
                                  "schedule entries"));
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
                                  mlsl_datatype_internal_t dtype, size_t dest)
{
    mlsl_status_t status = mlsl_status_success;
    mlsl_sched_entry *e = NULL;

    status = mlsl_sched_add_entry(sched, NULL, &e);
    MLSL_ASSERT(status == mlsl_status_success);

    size_t global_rank = 0;
    status = mlsl_comm_get_global_rank(sched->coll_param.comm, dest, &global_rank);
    MLSL_ASSERT(status == mlsl_status_success);

    e->type = mlsl_sched_entry_send;
    e->status = mlsl_sched_entry_status_not_started;
    e->is_barrier = 0;

    e->u.send.buf = buf;
    e->u.send.count = count;
    e->u.send.dtype = dtype;
    e->u.send.dest = global_rank;
    // TODO_ATL: return back when atl_request_t will be replaced by handle
    //e->u.send.req = NULL;      /* will be populated by _start_entry */
    e->u.send.comm = global_data.comm;
    memset(&e->u.send.req, 0, sizeof(e->u.send.req));

    return status;
}

mlsl_status_t mlsl_sched_add_recv(mlsl_sched *sched, void *buf, size_t count,
                                  mlsl_datatype_internal_t dtype, size_t src)
{
    mlsl_status_t status = mlsl_status_success;
    mlsl_sched_entry *e = NULL;

    status = mlsl_sched_add_entry(sched, NULL, &e);
    MLSL_ASSERT(status == mlsl_status_success);

    size_t global_rank = 0;
    status = mlsl_comm_get_global_rank(sched->coll_param.comm, src, &global_rank);
    MLSL_ASSERT(status == mlsl_status_success);

    e->type = mlsl_sched_entry_recv;
    e->status = mlsl_sched_entry_status_not_started;
    e->is_barrier = 0;

    e->u.recv.buf = buf;
    e->u.recv.count = count;
    e->u.recv.dtype = dtype;
    e->u.recv.src = global_rank;
    //e->u.recv.req = NULL;      /* will be populated by _start_entry */
    e->u.recv.comm = global_data.comm;

    memset(&e->u.recv.req, 0, sizeof(e->u.recv.req));

    return status;
}

mlsl_status_t mlsl_sched_add_reduce(mlsl_sched *sched, const void *in_buf, size_t in_count,
                                    void *inout_buf, size_t *out_count,
                                    mlsl_datatype_internal_t dtype, mlsl_reduction_t op)
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
    MLSL_ASSERTP(((op == mlsl_reduction_custom) && sched->coll_attr.reduction_fn) ||
                 ((op != mlsl_reduction_custom) && !sched->coll_attr.reduction_fn));
    reduce->op = op;
    reduce->reduction_fn = sched->coll_attr.reduction_fn;

    return status;
}

mlsl_status_t mlsl_sched_add_recv_reduce(mlsl_sched *sched, void *inout_buf, size_t count,
                                         size_t *out_count, mlsl_datatype_internal_t dtype,
                                         mlsl_reduction_t op, size_t src,
                                         void* comm_buf)
{
    mlsl_status_t status = mlsl_status_success;
    mlsl_sched_entry *e = NULL;

    status = mlsl_sched_add_entry(sched, NULL, &e);
    MLSL_ASSERT(status == mlsl_status_success);
    MLSL_ASSERT(inout_buf != NULL);

    size_t global_rank = 0;
    status = mlsl_comm_get_global_rank(sched->coll_param.comm, src, &global_rank);
    MLSL_ASSERT(status == mlsl_status_success);

    e->type = mlsl_sched_entry_recv_reduce;
    e->status = mlsl_sched_entry_status_not_started;
    e->is_barrier = 0;

    mlsl_sched_recv_reduce *recv_reduce = &e->u.recv_reduce;
    recv_reduce->inout_buf = inout_buf;
    recv_reduce->in_count = count;
    recv_reduce->out_count = out_count;
    recv_reduce->dtype = dtype;
    MLSL_ASSERTP(((op == mlsl_reduction_custom) && sched->coll_attr.reduction_fn) ||
                 ((op != mlsl_reduction_custom) && !sched->coll_attr.reduction_fn));
    recv_reduce->op = op;
    recv_reduce->reduction_fn = sched->coll_attr.reduction_fn;
    recv_reduce->src = global_rank;
    recv_reduce->comm = global_data.comm;
    if (comm_buf == NULL || comm_buf == inout_buf)
    {
        recv_reduce->comm_buf = MLSL_MALLOC(count * mlsl_datatype_get_size(dtype), "recv_reduce.comm_buf");
        mlsl_sched_add_persistent_memory(sched, mlsl_sched_memory_buffer, recv_reduce->comm_buf);
    }
    else
    {
        recv_reduce->comm_buf = comm_buf;
    }

    memset(&recv_reduce->req, 0, sizeof(recv_reduce->req));

    return status;
}

mlsl_status_t mlsl_sched_add_copy(mlsl_sched *sched, const void *in_buf,
                                  void *out_buf, size_t count, mlsl_datatype_internal_t dtype)
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

mlsl_status_t mlsl_sched_add_sync(mlsl_sched *sched, mlsl_sched_entry **sync_entry, int* entry_idx)
{
    mlsl_status_t status = mlsl_status_success;
    mlsl_sched_entry *e = NULL;
    mlsl_sched_sync *sync = NULL;
    status = mlsl_sched_add_entry(sched, entry_idx, &e);
    MLSL_ASSERT(status == mlsl_status_success);

    e->type = mlsl_sched_entry_sync;
    e->status = mlsl_sched_entry_status_not_started;
    e->is_barrier = 1;
    sync = &e->u.sync;
    sync->counter = 0;

    *sync_entry = e;

    return status;
}

mlsl_status_t mlsl_sched_add_collective(mlsl_sched *sched, mlsl_coll_type ctype,
                                        const void* send_buf, void* recv_buf, size_t count,
                                        mlsl_datatype_internal_t dtype, mlsl_reduction_t op)
{
    mlsl_status_t status = mlsl_status_success;
    mlsl_sched_entry *e = NULL;
    mlsl_sched_collective *coll = NULL;

    status = mlsl_sched_add_entry(sched, NULL, &e);
    MLSL_ASSERT(status == mlsl_status_success);

    e->type = mlsl_sched_entry_collective;
    e->status = mlsl_sched_entry_status_not_started;
    e->is_barrier = 1;
    coll = &e->u.coll;

    coll->ctype = ctype;
    coll->send_buf = send_buf;
    coll->recv_buf = recv_buf;
    coll->count = count;
    coll->dtype = dtype;
    coll->op = op;

    return status;
}

mlsl_status_t mlsl_sched_add_prologue(mlsl_sched *sched, mlsl_prologue_fn_t fn, const void* in_buf,
                                      size_t in_count, mlsl_datatype_internal_t in_dtype,
                                      void** out_buf, size_t* out_count, mlsl_datatype_internal_t out_dtype)
{
    mlsl_status_t status = mlsl_status_success;
    mlsl_sched_entry *e = NULL;
    mlsl_sched_prologue *prologue = NULL;

    status = mlsl_sched_add_entry(sched, NULL, &e);
    MLSL_ASSERT(status == mlsl_status_success);

    e->type = mlsl_sched_entry_prologue;
    e->status = mlsl_sched_entry_status_not_started;
    e->is_barrier = 0;
    prologue = &e->u.prologue;

    prologue->fn = fn;
    prologue->in_buf = in_buf;
    prologue->in_count = in_count;
    prologue->in_dtype = in_dtype;
    prologue->out_buf = out_buf;
    prologue->out_count = out_count;
    prologue->out_dtype = (mlsl_datatype_internal*)out_dtype;

    return status;
}

mlsl_status_t mlsl_sched_add_epilogue(mlsl_sched *sched, mlsl_epilogue_fn_t fn, const void* in_buf, size_t in_count,
                                      mlsl_datatype_internal_t in_dtype, void* out_buf, size_t* out_count,
                                      size_t expected_out_count, mlsl_datatype_internal_t out_dtype)
{
    mlsl_status_t status = mlsl_status_success;
    mlsl_sched_entry *e = NULL;
    mlsl_sched_epilogue *epilogue = NULL;

    status = mlsl_sched_add_entry(sched, NULL, &e);
    MLSL_ASSERT(status == mlsl_status_success);

    e->type = mlsl_sched_entry_epilogue;
    e->status = mlsl_sched_entry_status_not_started;
    e->is_barrier = 0;
    epilogue = &e->u.epilogue;

    epilogue->fn = fn;
    epilogue->in_buf = in_buf;
    epilogue->in_count = in_count;
    epilogue->in_dtype = in_dtype;
    epilogue->out_buf = out_buf;
    epilogue->out_count = out_count;
    epilogue->expected_out_count = expected_out_count;
    epilogue->out_dtype = out_dtype;

    return status;
}

mlsl_status_t mlsl_sched_add_update_postponed_fields(mlsl_sched *sched, size_t part_idx, size_t part_count)
{
    mlsl_status_t status = mlsl_status_success;
    mlsl_sched_entry *e = NULL;
    mlsl_sched_update_postponed_fields *update_fields = NULL;

    status = mlsl_sched_add_entry(sched, NULL, &e);
    MLSL_ASSERT(status == mlsl_status_success);

    e->type = mlsl_sched_entry_update_postponed_fields;
    e->status = mlsl_sched_entry_status_not_started;
    e->is_barrier = 0;
    update_fields = &e->u.update_fields;

    update_fields->part_idx = part_idx;
    update_fields->part_count = part_count;
    
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
            /* TODO: do we need special handling for first_progress ? */
            MLSL_LOG(DEBUG, "do initial mlsl_sched_continue");
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
                        }
                    }
                    break;
                case mlsl_sched_entry_recv:
                    if (e->status == mlsl_sched_entry_status_started) {
                        atl_comm_check(bin->comm_ctx, &req_status, &e->u.recv.req);
                        if (req_status) {
                            MLSL_LOG(DEBUG, "completed RECV entry %zu, rreq=%p", i, &e->u.recv.req);
                            e->status = mlsl_sched_entry_status_complete;
                        }
                    }
                    break;
                case mlsl_sched_entry_recv_reduce:
                    if (e->status == mlsl_sched_entry_status_started) {
                        atl_comm_check(bin->comm_ctx, &req_status, &e->u.recv_reduce.req);
                        if (req_status) {
                            MLSL_LOG(DEBUG, "completed RECV in RECV_REDUCE entry %zu, rreq=%p", i, &e->u.recv_reduce.req);
                            mlsl_sched_start_entry(s, i, e);
                            e->status = mlsl_sched_entry_status_complete;
                        }
                    }
                    break;
                case mlsl_sched_entry_sync:
                case mlsl_sched_entry_collective:
                    if (e->status == mlsl_sched_entry_status_started) {
                        mlsl_sched_start_entry(s, i, e);
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
    sched->sched_id = mlsl_comm_get_sched_id(sched->coll_param.comm);
    mlsl_request *req;
    mlsl_request_create(&req);
    req->sched = sched;
    sched->req = req;

    mlsl_parallelizer_process(global_data.parallelizer, sched, &sched->partial_scheds,
                              &sched->partial_sched_count);

    MLSL_LOG(DEBUG, "sched %p, num_entries %zu, size %zu, number %u, req %p, part_scheds %p, part_count %zu",
             sched, sched->num_entries, sched->size, sched->sched_id, req, sched->partial_scheds, sched->partial_sched_count);

    return mlsl_status_success;
}

mlsl_status_t mlsl_sched_add_persistent_memory(mlsl_sched *sched, mlsl_sched_memory_type type, void *ptr)
{
    MLSL_LOG(DEBUG, "sched %p, type %d, ptr %p", sched, type, ptr);

    mlsl_sched_memory *mem = static_cast<mlsl_sched_memory*>(MLSL_CALLOC(sizeof(mlsl_sched_memory), "sched_memory"));
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

mlsl_status_t mlsl_sched_set_coll_attr(mlsl_sched *sched, const mlsl_coll_attr_t *attr)
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

mlsl_status_t mlsl_sched_bcast(
    void *buf,
    size_t count,
    mlsl_datatype_internal_t dtype,
    size_t root,
    mlsl_comm* comm,
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
    p->comm = comm ? comm : global_data.comm;

    return status;
}

mlsl_status_t mlsl_sched_reduce(
    const void *send_buf,
    void *recv_buf,
    size_t count,
    mlsl_datatype_internal_t dtype,
    mlsl_reduction_t reduction,
    size_t root,
    mlsl_comm* comm,
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
    p->comm = comm ? comm : global_data.comm;

    return status;
}

mlsl_status_t mlsl_sched_allreduce(
    const void *send_buf,
    void *recv_buf,
    size_t count,
    mlsl_datatype_internal_t dtype,
    mlsl_reduction_t reduction,
    mlsl_comm* comm,
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
    p->comm =  comm ? comm : global_data.comm;

    return status;
}

mlsl_status_t mlsl_sched_allgatherv(
    const void *send_buf,
    size_t send_count,
    void *recv_buf,
    size_t *recv_counts,
    mlsl_datatype_internal_t dtype,
    mlsl_comm* comm,
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
    p->comm =  comm ? comm : global_data.comm;

    return status;
}

mlsl_status_t mlsl_sched_barrier(mlsl_comm* comm, mlsl_sched **sched)
{
    mlsl_status_t status = mlsl_status_success;

    MLSL_CALL(mlsl_sched_create(sched));

    mlsl_sched_coll_param *p = &((*sched)->coll_param);
    p->ctype = mlsl_coll_barrier;
    p->dtype = mlsl_dtype_internal_char;
    p->comm =  comm ? comm : global_data.comm;

    return status;
}
