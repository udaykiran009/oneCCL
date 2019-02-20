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
    MLSL_LOG(DEBUG, "sched %p, prio %zu", sched, priority);
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

mlsl_status_t mlsl_sched_progress(mlsl_sched_queue_bin* bin,
                                  size_t max_sched_count,
                                  size_t& completed_sched_count)
{
    mlsl_status_t status = mlsl_status_success;
    mlsl_request *req __attribute__ ((unused));
    size_t sched_count = 0;

    completed_sched_count = 0;

    MLSL_LOG(TRACE, "bin %p, elems count %zu, max scheds %zu",
             bin, bin->elems.size(), max_sched_count);

    /* ensure communication progress */
    atl_status_t atl_status __attribute__ ((unused));
    atl_status = atl_comm_poll(bin->comm_ctx);
    MLSL_ASSERT(atl_status == atl_status_success);

    //iterate through the scheds store in the bin
    for(auto it = bin->elems.begin(); it != bin->elems.end(); )
    {
        mlsl_sched* sched = *it;
        if (sched->first_progress)
        {
            //perform initial progress, iterate throught schedule entries
            //some entries are running in 'synchronous' way, they are completed right after execution
            //other entries are running in 'asynchronous' way and may not be completed right after execution
            //entry N+1 may be started right after entry N even if entry N has not been completed
            //if entry N+1 (and all subsequent entries) depends on entry N, then entry N is marked as a barrier and
            //      entry N+1 (and all subsequent) won't be started until entry N is completed
            /* TODO: do we need special handling for first_progress ? */
            MLSL_LOG(DEBUG, "do initial mlsl_sched_continue for sched %p", sched);
            status = mlsl_sched_continue(sched);
            MLSL_ASSERT(status == mlsl_status_success);
            sched->first_progress = false;
        }

        //continue iteration of already started schedule. @b sched->idx is an index of the last completed entry
        for (auto entry_idx = sched->idx; entry_idx < sched->entries.size(); ++entry_idx)
        {
            auto& entry = sched->entries[entry_idx];
            //check for completion of 'asynchronous' entries
            entry->update();
            if (entry_idx == sched->idx && entry->get_status() >= mlsl_sched_entry_status_complete)
            {
                //the entry has been completed, increment the index
                ++sched->idx;
                MLSL_LOG(DEBUG, "completed %s%s entry [%zu/%zu], shift start_idx, sched %p", entry->name(),
                         entry->is_barrier() ? " barrier" : "", entry_idx, sched->entries.size(), sched);
                if (entry->is_barrier())
                {
                    //that entry was marked as a barrier, run the rest entries (if any) which depend on it
                    status = mlsl_sched_continue(sched);
                    MLSL_ASSERT(status == mlsl_status_success);
                }
            }
            else if (entry->is_barrier() && entry->get_status() < mlsl_sched_entry_status_complete)
            {
                //the entry has not been completed yet. It is marked as a barrier, don't process anything after it
                break;
            }
        }

        if (sched->idx == sched->entries.size())
        {
            //the last entry in the schedule has been completed, clean up the schedule and complete its request
            MLSL_LOG(DEBUG, "completing and dequeuing: sched %p, req %p", sched, sched->req);

            // remove completed schedule from the bin. Iterator @b it will point to the next elem in bin->elems
            it = bin->queue->erase(bin, it);

            req = sched->req;
            sched->req = nullptr;

            status = mlsl_request_complete(req);
            MLSL_ASSERT(status == mlsl_status_success);

            ++completed_sched_count;
        }
        else
        {
            //this schedule is not completed yet, switch to the next sched in bin elems list
            //progression of unfinished schedules will be continued in the next call of @ref mlsl_sched_progress
            ++it;
        }

        sched_count++;
        if (sched_count == max_sched_count)
        {
            //desired number of processed scheds is reached, exit
            break;
        }
    }

    return status;
}

void mlsl_sched::commit(mlsl_parallelizer* parallelizer)
{
    sched_id = coll_param.comm->get_sched_id();

    mlsl_request_create(&req);
    req->sched = this;

    mlsl_parallelizer_process(parallelizer, this);

    MLSL_LOG(DEBUG, "sched %p, num_entries %zu, number %u, req %p, part_count %zu",
             this, entries.size(), sched_id, req, partial_scheds.size());
}

mlsl_request* mlsl_sched::start(mlsl_executor* exec)
{
    /* sanity check the schedule */
    MLSL_ASSERT(entries.empty() || idx < entries.size());
    MLSL_ASSERT(req);
    MLSL_ASSERT(coll_param.comm);

    MLSL_LOG(DEBUG, "starting schedule %p, type %s", this, mlsl_coll_type_to_str(coll_param.ctype));

    prepare_partial_scheds(exec->proc_idx == 0);

    exec->start_sched(this);

    return req;
}

void mlsl_sched::prepare_partial_scheds(bool dump_scheds)
{
    size_t partial_sched_count = partial_scheds.size();
    MLSL_ASSERTP(partial_sched_count > 0);

    for (size_t idx = 0; idx < partial_sched_count; idx++)
    {
        partial_scheds[idx]->update_id();
        partial_scheds[idx]->reset();
        if (dump_scheds)
        {
            partial_scheds[idx]->dump("worker_sched");
        }
    }

    if (dump_scheds)
    {
        dump("origin_sched");
    }

    reset_request();
}

void mlsl_sched::reset()
{
    idx = 0;
    first_progress = true;
    for (auto& entry :  entries)
    {
        entry->reset();
    }
}

mlsl_request* mlsl_sched::reset_request()
{
    req->completion_counter = static_cast<int>(partial_scheds.size());

    for(auto& part_sched : partial_scheds)
    {
        part_sched->req = req;
    }

    return req;
}

void mlsl_sched::add_barrier()
{
    /* mark the last entry as a barrier unless we're at the beginning, which
     * would be a pointless barrier */
    if (!entries.empty())
    {
        entries.back()->make_barrier();
    }
}

void mlsl_sched::sync_partial_scheds()
{
    if(!partial_scheds.empty())
    {
        auto sync_obj = std::make_shared<sync_object>(partial_scheds.size());

        for(auto& sched : partial_scheds)
        {
            entry_factory::make_sync_entry(sched.get(), sync_obj);
        }
    }
}

void mlsl_sched::set_coll_attr(const mlsl_coll_attr_t *attr)
{
    MLSL_ASSERT(attr);
    coll_attr.prologue_fn = attr->prologue_fn;
    coll_attr.epilogue_fn = attr->epilogue_fn;
    coll_attr.reduction_fn = attr->reduction_fn;
    coll_attr.priority = attr->priority;
    coll_attr.synchronous = attr->synchronous;
    coll_attr.to_cache = attr->to_cache;
    if (attr->match_id)
    {
        strncpy(coll_attr.match_id, attr->match_id, MLSL_MATCH_ID_MAX_LEN - 1);
    }
}

void mlsl_sched::dump(const char *name) const
{
    if (!env_data.sched_dump)
        return;

    const size_t bytes_per_sched_entry = 2048; //some heuristic value
    std::vector<char> buf (bytes_per_sched_entry * (entries.size() + 1));
    char* write_buf = buf.data();

    write_buf += sprintf(write_buf, "\n--------------------------------\n");
    write_buf += sprintf(write_buf, "sched: %s, coll %s, %p, start_idx %zu, "
                                    "num_entries %zu, comm_id %u, sched_id %u, req %p\n",
                         name, mlsl_coll_type_to_str(coll_param.ctype), this, idx,
                         entries.size(), coll_param.comm->id(), sched_id, req);

    for (size_t i = 0; i < entries.size(); ++i)
    {
        write_buf = entries[i]->dump(write_buf, i);
    }
    sprintf(write_buf, "--------------------------------\n");
    MLSL_LOG(INFO, "%s", buf.data());
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
    for (auto& part_sched: partial_scheds)
    {
        part_sched.reset();
    }

    if (req)
        mlsl_request_free(req);

    if (!memory.mr_list.empty())
    {
        auto dereg_sched = new mlsl_sched{};
        dereg_sched->coll_attr.to_cache = false;
        entry_factory::make_deregister_entry(dereg_sched, memory.mr_list);

        mlsl_request *dereg_req;
        mlsl_sched_start_subsched(this, dereg_sched, &dereg_req);
        mlsl_wait(dereg_req);
        MLSL_ASSERTP(memory.mr_list.empty());
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
    std::swap(root, other.root);
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
    subsched->reset();
    subsched->dump("subsched");
    sched->bin->queue->add(subsched, mlsl_sched_get_priority(sched));
    *r = req;
    return mlsl_status_success;
}
