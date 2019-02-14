#include "sched/sched.hpp"
#include "sched/sched_queue.hpp"
#include "sched/sync_object.hpp"
#include "sched/entry_factory.hpp"
#include "comp/comp.hpp"
#include "common/global/global.hpp"
#include "common/log/log.hpp"
#include "common/utils/utils.hpp"
#include "common/comm/comm.hpp"
#include "out_of_order/ooo_match.hpp"

#include <stdio.h>
#include <string.h>

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

const char *mlsl_reduction_to_str(mlsl_reduction_t type)
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
    char* write_buf = buf;

    write_buf += sprintf(write_buf, "\n--------------------------------\n");
    if (s)
    {
        write_buf += sprintf(write_buf, "sched: %s, coll %s, %p, start_idx %zu, "
                             "num_entries %zu, sched_id %u, req %p\n",
                             name, mlsl_coll_type_to_str(s->coll_param.ctype), s, s->idx,
                             s->entries.size(), s->sched_id, s->req);

        for (i = 0; i < s->entries.size(); ++i)
        {
            write_buf = s->entries[i]->dump(write_buf, i);
        }
    }
    write_buf += sprintf(write_buf, "--------------------------------\n");
    MLSL_LOG(INFO, "%s", buf);

    return mlsl_status_success;
}

/* Posts or performs any NOT_STARTED operations in the given schedule that are
 * permitted to be started.  That is, this routine will respect schedule
 * barriers appropriately. */
static mlsl_status_t mlsl_sched_continue(mlsl_sched *s)
{
    mlsl_status_t status = mlsl_status_success;
    size_t i;

    for (i = s->idx; i < s->entries.size(); ++i)
    {
        auto& entry = s->entries[i];

        if (entry->get_status() == mlsl_sched_entry_status_not_started) {
            MLSL_LOG(DEBUG, "starting entry %s [%zu/%zu]", entry->name(), i, s->entries.size());
            entry->start();
            entry = s->entries[i];
        }

        /* _start_entry may have completed the operation, but won't update s->idx */
        if (i == s->idx && entry->get_status() >= mlsl_sched_entry_status_complete) {
            ++s->idx;   /* this is valid even for barrier entries */
            MLSL_LOG(DEBUG, "completed %s%s entry [%zu/%zu], shift start_idx, sched %p", entry->name(),
                         entry->is_barrier() ? " barrier" : "", i, s->entries.size(), s);
        }

        /* watch the indexing, s->idx might have been incremented above, so
         * ||-short-circuit matters here */
        if (entry->is_barrier() && (entry->get_status() < mlsl_sched_entry_status_complete || (s->idx != i + 1))) {
            /* we've hit a barrier but outstanding operations before this
             * barrier remain, so we cannot proceed past the barrier */
            break;
        }
    }

    return status;
}

mlsl_status_t mlsl_sched_update_id(mlsl_sched *sched)
{
    sched->sched_id = sched->coll_param.comm->get_sched_id();
    return mlsl_status_success;
}

mlsl_status_t mlsl_sched_reset(mlsl_sched *sched)
{
    sched->idx = 0;
    sched->first_progress = true;
    for (size_t idx = 0; idx < sched->entries.size(); idx++)
    {
        sched->entries[idx]->reset();
    }
    return mlsl_status_success;
}

mlsl_status_t mlsl_sched_sync_schedules(mlsl_sched **scheds, size_t count)
{
    auto sync_obj = std::make_shared<sync_object>(count);

    for(size_t idx = 0; idx < count; ++idx)
    {
        entry_factory::make_sync_entry(scheds[idx], sync_obj);
    }

    return mlsl_status_success;
}

void mlsl_sched_reset_request(mlsl_sched *s, mlsl_request **req)
{
    s->req->completion_counter = s->partial_sched_count;
    for(size_t idx = 0; idx < s->partial_sched_count; ++idx)
    {
        s->partial_scheds[idx]->req = s->req;
    }

    if (req)
        *req = s->req;
}

mlsl_status_t mlsl_sched_start(mlsl_sched *s, mlsl_request **req)
{
    MLSL_ASSERTP(s && req);

    /* sanity check the schedule */
    MLSL_ASSERT(s->entries.size() == 0 || s->idx < s->entries.size());
    MLSL_ASSERT(s->req != NULL);
    MLSL_ASSERT(s->coll_param.comm);

    MLSL_LOG(DEBUG, "starting schedule %p, type %s", s, mlsl_coll_type_to_str(s->coll_param.ctype));
    global_data.executor->start_sched(s);

    *req = s->req;

    return mlsl_status_success;
}

/* require that all previously added ops are complete before subsequent ops
 * may begin to execute */
mlsl_status_t mlsl_sched_add_barrier(mlsl_sched *sched)
{
    mlsl_status_t status = mlsl_status_success;

    /* mark the previous entry as a barrier unless we're at the beginning, which
     * would be a pointless barrier */
    if (sched->entries.size() > 0) {
        sched->entries[sched->entries.size() - 1]->make_barrier();
    }

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
            MLSL_LOG(DEBUG, "do initial mlsl_sched_continue for sched %p", s);
            status = mlsl_sched_continue(s);
            MLSL_ASSERT(status == mlsl_status_success);
            s->first_progress = false;
        }

        for (i = s->idx; i < s->entries.size(); ++i) {
            auto& entry = s->entries[i];
            entry->update();
            if (i == s->idx && entry->get_status() >= mlsl_sched_entry_status_complete)
            {
                ++s->idx;
                MLSL_LOG(DEBUG, "completed %s%s entry [%zu/%zu], shift start_idx, sched %p", entry->name(),
                         entry->is_barrier() ? " barrier" : "", i, s->entries.size(), s);
                if (entry->is_barrier()) {
                    /* post/perform the next round of operations */
                    status = mlsl_sched_continue(s);
                    MLSL_ASSERT(status == mlsl_status_success);
                }
            }
            else if (entry->is_barrier() && entry->get_status() < mlsl_sched_entry_status_complete)
            {
                /* don't process anything after this barrier entry */
                break;
            }
        }

        if (s->idx == s->entries.size()) {
            MLSL_LOG(DEBUG, "completing and dequeuing: sched %p, req %p", s, s->req);

            /* dequeue this schedule, it's complete */
            bin->queue->erase(bin, s);

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
    sched->sched_id = sched->coll_param.comm->get_sched_id();
    mlsl_request *req;
    mlsl_request_create(&req);
    req->sched = sched;
    sched->req = req;

    mlsl_parallelizer_process(global_data.parallelizer, sched, &sched->partial_scheds,
                              &sched->partial_sched_count);

    MLSL_LOG(DEBUG, "sched %p, num_entries %zu, number %u, req %p, part_scheds %p, part_count %zu",
             sched, sched->entries.size(), sched->sched_id, req, sched->partial_scheds, sched->partial_sched_count);

    return mlsl_status_success;
}

mlsl_status_t mlsl_sched_alloc_buffer(mlsl_sched* sched, size_t size, void** ptr)
{
    MLSL_LOG(DEBUG, "sched %p, size %zu", sched, size);
    MLSL_ASSERTP(size > 0);
    MLSL_ASSERTP(ptr);
    void* p = MLSL_CALLOC(size, "sched_buffer");
    sched->memory.buf_list.emplace_back(p, size);
    *ptr = p;
    return mlsl_status_success;
}

mlsl_status_t mlsl_sched_free_buffers(mlsl_sched* sched)
{
    MLSL_LOG(DEBUG, "sched %p", sched);
    std::list<mlsl_sched_buffer_handler>::iterator it;
    for (it = sched->memory.buf_list.begin(); it != sched->memory.buf_list.end(); it++)
    {
        MLSL_LOG(DEBUG, "free %p", it->ptr);
        MLSL_FREE(it->ptr);
    }
    sched->memory.buf_list.clear();
    return mlsl_status_success;
}

mlsl_status_t mlsl_sched_start_subsched(mlsl_sched* sched, mlsl_sched* subsched, mlsl_request **r)
{
    mlsl_request* req;
    mlsl_request_create(&req);
    req->completion_counter = 1;
    req->sched = subsched;
    subsched->req = req;
    subsched->sched_id = sched->sched_id;
    mlsl_sched_reset(subsched);
    mlsl_sched_dump(subsched, "subsched");
    sched->bin->queue->add(subsched, mlsl_sched_get_priority(sched));
    *r = req;
    return mlsl_status_success;
}

mlsl_status_t mlsl_sched_set_entry_exec_mode(mlsl_sched* sched, mlsl_sched_entry_exec_mode mode)
{
    sched->exec_mode = mode;
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

mlsl_status_t mlsl_sched_bcast(
    void *buf,
    size_t count,
    mlsl_datatype_internal_t dtype,
    size_t root,
    mlsl_comm* comm,
    mlsl_sched **sched)
{
    mlsl_status_t status = mlsl_status_success;

    *sched = new mlsl_sched{};

    mlsl_sched_coll_param *p = &((*sched)->coll_param);
    p->ctype = mlsl_coll_bcast;
    p->buf = buf;
    p->count = count;
    p->dtype = dtype;
    p->root = root;
    p->comm = comm ? comm : global_data.comm.get();

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

    *sched = new mlsl_sched{};

    mlsl_sched_coll_param *p = &((*sched)->coll_param);
    p->ctype = mlsl_coll_reduce;
    p->send_buf = send_buf;
    p->recv_buf = recv_buf;
    p->count = count;
    p->dtype = dtype;
    p->reduction = reduction;
    p->root = root;
    p->comm = comm ? comm : global_data.comm.get();

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

    *sched = new mlsl_sched{};

    mlsl_sched_coll_param *p = &((*sched)->coll_param);
    p->ctype = mlsl_coll_allreduce;
    p->send_buf = send_buf;
    p->recv_buf = recv_buf;
    p->count = count;
    p->dtype = dtype;
    p->reduction = reduction;
    p->comm =  comm ? comm : global_data.comm.get();

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

    *sched = new mlsl_sched{};

    mlsl_sched_coll_param *p = &((*sched)->coll_param);
    p->ctype = mlsl_coll_allgatherv;
    p->send_buf = send_buf;
    p->send_count = send_count;
    p->recv_buf = recv_buf;
    p->recv_counts = recv_counts;
    p->dtype = dtype;
    p->comm =  comm ? comm : global_data.comm.get();

    return status;
}

mlsl_status_t mlsl_sched_barrier(mlsl_comm* comm, mlsl_sched **sched)
{
    mlsl_status_t status = mlsl_status_success;

    *sched = new mlsl_sched{};

    mlsl_sched_coll_param *p = &((*sched)->coll_param);
    p->ctype = mlsl_coll_barrier;
    p->dtype = mlsl_dtype_internal_char;
    p->comm =  comm ? comm : global_data.comm.get();

    return status;
}

mlsl_status_t mlsl_sched_tensor_bcast(mlsl_comm* comm, mlsl_sched** sched, bool temporal)
{
    MLSL_ASSERT(comm != nullptr);
    mlsl_status_t status = mlsl_status_success;

    *sched = new mlsl_sched{};

    mlsl_sched_coll_param *p = &((*sched)->coll_param);
    p->ctype = temporal ? mlsl_coll_service_temporal : mlsl_coll_service_persistent;
    p->dtype = mlsl_dtype_internal_char;
    p->comm =  comm;

    return status;
}

void mlsl_sched_prepare_partial_scheds(mlsl_sched *sched, bool dump)
{
    mlsl_sched **partial_scheds = sched->partial_scheds;
    size_t partial_sched_count = sched->partial_sched_count;
    MLSL_ASSERTP(partial_scheds && partial_sched_count > 0);

    size_t idx;
    for (idx = 0; idx < partial_sched_count; idx++)
    {
        mlsl_sched_update_id(partial_scheds[idx]);
        mlsl_sched_reset(partial_scheds[idx]);

        if (dump)
        {
            mlsl_sched_dump(partial_scheds[idx], "worker_sched");
        }
    }

    if (dump)
    {
        mlsl_sched_dump(sched, "origin_sched");
    }

    mlsl_sched_reset_request(sched, NULL);
}

mlsl_sched& mlsl_sched::operator=(const mlsl_sched& other)
{
    if(this != &other)
    {
        mlsl_sched(other).swap(*this);
    }
    return *this;
}

mlsl_sched::~mlsl_sched()
{
    for (idx = 0; idx < partial_sched_count; idx++)
    {
        delete partial_scheds[idx];
    }
    MLSL_FREE(partial_scheds);

    if (req)
        mlsl_request_free(req);

    if (memory.mr_list.size())
    {
        mlsl_sched* dereg_sched = new mlsl_sched{};
        dereg_sched->coll_attr.to_cache = false;
        entry_factory::make_deregister_entry(dereg_sched, memory.mr_list);

        mlsl_request *dereg_req;
        mlsl_sched_start_subsched(this, dereg_sched, &dereg_req);
        mlsl_wait(dereg_req);
        MLSL_ASSERTP(memory.mr_list.size() == 0);
    }
    mlsl_sched_free_buffers(this);
}

void mlsl_sched::swap(mlsl_sched& other)
{
    MLSL_ASSERTP(0);
    std::swap(first_progress, other.first_progress);
    std::swap(bin, other.bin);
    std::swap(coll_param, other.coll_param);
    std::swap(coll_attr, other.coll_attr);
    std::swap(idx, other.idx);
    std::swap(entries, other.entries);
    std::swap(sched_id, other.sched_id);
    std::swap(req, other.req);
    std::swap(memory, other.memory);
    std::swap(exec_mode, other.exec_mode);
    std::swap(partial_scheds, other.partial_scheds);
    std::swap(partial_sched_count, other.partial_sched_count);
    std::swap(root, other.root);
    std::swap(next, other.next);
    std::swap(prev, other.prev);
}
