#include "sched/sched.hpp"
#include "sched/sched_queue.hpp"
#include "sched/sync_object.hpp"
#include "sched/entry_factory.hpp"
#include "common/global/global.hpp"
#include "out_of_order/ooo_match.hpp"
#include "parallelizer/parallelizer.hpp"

static size_t lifo_priority = 0;

void mlsl_sched::alloc_req()
{
    req = new mlsl_request();
    req->sched = this;
}

mlsl_sched::~mlsl_sched()
{
    if (finalize_fn)
    {
        finalize_fn(this, finalize_fn_ctx);
    }

    for (auto& part_sched: partial_scheds)
    {
        part_sched.reset();
    }

    if (is_own_req)
    {
        LOG_DEBUG("delete own req");
        delete req;
        req = nullptr;
    }

    if (!memory.mr_list.empty())
    {
        /* perform deregistration in worker thread */
        mlsl_coll_param cparam{};
        cparam.ctype = mlsl_coll_none;
        cparam.comm = coll_param.comm;
        mlsl_sched* dereg_sched = new mlsl_sched(cparam);
        entry_factory::make_deregister_entry(dereg_sched, memory.mr_list);
        if (global_data.is_worker_thread || !env_data.worker_offload)
        {
            dereg_sched->do_progress();
            delete dereg_sched;
        }
        else
        {
            mlsl_wait(start_subsched(dereg_sched));
        }
        if(!memory.mr_list.empty())
        {
            LOG_ERROR("memory list is not empty");
        }
        MLSL_ASSERT(memory.mr_list.empty());
    }

    free_buffers();
}

size_t mlsl_sched::get_priority()
{
    size_t priority = 0;
    switch (env_data.priority_mode)
    {
        case mlsl_priority_none:
            priority = 0;
            break;
        case mlsl_priority_direct:
        case mlsl_priority_lifo:
            priority = coll_attr.priority;
            break;
        default:
            MLSL_FATAL("unexpected priority_mode ", env_data.priority_mode);
            break;
    }

    LOG_DEBUG("sched, ", this, ", prio ", priority);
    return priority;
}

/* Posts or performs any NOT_STARTED operations in the given schedule that are
 * permitted to be started.  That is, this routine will respect schedule
 * barriers appropriately. */
void mlsl_sched::do_progress()
{
    for (size_t i = start_idx; i < entries.size(); ++i)
    {
        auto& entry = entries[i];

        if (entry->get_status() == mlsl_sched_entry_status_not_started)
        {
            LOG_DEBUG("starting entry ", entry->name(), "[", i, "/", entries.size(), "]");
            entry->start();
            entry = entries[i];
        }

        /* _start_entry may have completed the operation, but won't update start_idx */
        if (i == start_idx && entry->get_status() >= mlsl_sched_entry_status_complete)
        {
            ++start_idx;   /* this is valid even for barrier entries */
            LOG_DEBUG("completed ", entry->name(), entry->is_barrier() ? " barrier" : "",
                " entry [", i, "/", entries.size(), "] shift start_idx, sched ", this);
        }

        /* watch the indexing, start_idx might have been incremented above, so
         * ||-short-circuit matters here */
        if (entry->is_barrier() && (entry->get_status() < mlsl_sched_entry_status_complete || (start_idx != i + 1)))
        {
            /* we've hit a barrier but outstanding operations before this
             * barrier remain, so we cannot proceed past the barrier */
            break;
        }
    }
}

mlsl_status_t mlsl_bin_progress(mlsl_sched_bin* bin,
                                size_t max_sched_count,
                                size_t& completed_sched_count)
{
    MLSL_ASSERT(bin);

    mlsl_status_t status = mlsl_status_success;
    size_t sched_count = 0;
    auto sched_queue = bin->get_queue();
    size_t bin_size = bin->size();
    MLSL_ASSERT(bin_size > 0 && bin_size >= max_sched_count);

    LOG_TRACE("bin ", bin, ", sched_count ", bin->size(), ", max_scheds ", max_sched_count);

    /* ensure communication progress */
    atl_status_t atl_status __attribute__ ((unused));
    atl_status = atl_comm_poll(bin->get_comm_ctx());
    MLSL_THROW_IF_NOT(atl_status == atl_status_success, "bad status ", atl_status);

    // iterate through the scheds store in the bin
    completed_sched_count = 0;
    for (size_t sched_idx = 0; sched_idx < bin_size; )
    {
        mlsl_sched* sched = bin->get(sched_idx);
        MLSL_ASSERT(sched && bin == sched->bin);

        mlsl_sched_progress(sched);

        if (sched->start_idx == sched->entries.size())
        {
            // the last entry in the schedule has been completed, clean up the schedule and complete its request
            LOG_DEBUG("completing and dequeuing: sched ", sched,
                ", coll ", mlsl_coll_type_to_str(sched->coll_param.ctype),
                ", req ", sched->req,
                ", entry_count ", sched->entries.size());

            // remove completed schedule from the bin
            sched_idx = sched_queue->erase(bin, sched_idx);
            bin_size = bin->size();
            LOG_DEBUG("completing request ", sched->req);
            sched->complete();
            ++completed_sched_count;
        }
        else
        {
            // this schedule is not completed yet, switch to the next sched in bin scheds list
            // progression of unfinished schedules will be continued in the next call of @ref mlsl_bin_progress
            ++sched_idx;
        }
        sched_count++;

        if (sched_count == max_sched_count)
        {
            // desired number of processed scheds is reached, exit
            break;
        }
    }

    return status;
}

void mlsl_sched_progress(mlsl_sched* sched)
{
    if (sched->first_progress)
    {
        // perform initial progress, iterate through schedule entries
        // some entries are running in 'synchronous' way, they are completed right after execution
        // other entries are running in 'asynchronous' way and may not be completed right after execution
        // entry N+1 may be started right after entry N even if entry N has not been completed
        // if entry N+1 (and all subsequent entries) depends on entry N, then entry N is marked as a barrier and
        //     entry N+1 (and all subsequent) won't be started until entry N is completed
        /* TODO: do we need special handling for first_progress ? */
        LOG_DEBUG("initial do_progress for sched ", sched);
        sched->do_progress();
        sched->first_progress = false;
    }

    // continue iteration of already started schedule. @b start_idx is an index of the first non-started entry
    for (auto entry_idx = sched->start_idx; entry_idx < sched->entries.size(); ++entry_idx)
    {
        auto& entry = sched->entries[entry_idx];

        // check for completion of 'asynchronous' entries
        entry->update();

        if (entry_idx == sched->start_idx && entry->get_status() >= mlsl_sched_entry_status_complete)
        {
            // the entry has been completed, increment start_idx
            ++sched->start_idx;
            LOG_DEBUG("completed ", entry->name(), entry->is_barrier() ? " barrier" : "",
                      " entry [", entry_idx, "/", sched->entries.size(), "] shift start_idx, sched ", sched);
            if (entry->is_barrier())
            {
                // that entry was marked as a barrier, run the rest entries (if any) which depend on it
                sched->do_progress();
            }
        }
        else if (entry->is_barrier() && entry->get_status() < mlsl_sched_entry_status_complete)
        {
            // the entry has not been completed yet. It is marked as a barrier, don't process anything after it
            break;
        }
    }

}


void mlsl_sched::commit(mlsl_parallelizer* parallelizer)
{
    update_id();

    if (env_data.priority_mode == mlsl_priority_lifo)
    {
        coll_attr.priority = lifo_priority;
        lifo_priority++;
    }

    if (parallelizer)
        parallelizer->process(this);

    LOG_DEBUG("sched ", this, ", num_entries ", entries.size(), ", number ", sched_id, ", req ", req,
              ", part_count ", partial_scheds.size());
}

mlsl_request* mlsl_sched::start(mlsl_executor* exec, bool reset_sched)
{
    /* sanity check the schedule */
    MLSL_ASSERT(start_idx == 0);
    MLSL_ASSERT(req);
    MLSL_ASSERT(coll_param.comm);

    LOG_DEBUG("starting schedule ", this, ", type ", mlsl_coll_type_to_str(coll_param.ctype));

    reset();
    prepare_partial_scheds();

    if (reset_sched)
    {
        reset_request();
    }

    if (env_data.sched_dump)
    {
        dump_all();
    }

    exec->start(this);

    return req;
}

void mlsl_sched::complete()
{
#ifdef ENABLE_DEBUG
    exec_complete_time = timer_type::now();
    if(env_data.sched_dump)
    {
        dump("completed_sched");
    }
#endif

    req->complete();
}

void mlsl_sched::add_partial_sched(mlsl_coll_param& coll_param)
{
    partial_scheds.emplace_back(std::make_shared<mlsl_sched>(coll_param));
    partial_scheds.back()->internal_type = internal_type;
    partial_scheds.back()->set_request(req);
}

void mlsl_sched::set_request(mlsl_request* req)
{
    if (this->req)
        delete this->req;

    this->req = req;
    is_own_req = false;
}

void mlsl_sched::prepare_partial_scheds()
{
    for (auto& sched: partial_scheds)
    {
        sched->update_id();
        sched->reset();
    }
}

void mlsl_sched::reset()
{
#ifdef ENABLE_DEBUG
    exec_start_time = timer_type::now();
    exec_complete_time = exec_start_time;
#endif
    start_idx = 0;
    first_progress = true;
    for (auto& entry : entries)
    {
        entry->reset();
    }
}

mlsl_request* mlsl_sched::reset_request()
{
    int completion_counter = 0;
    if (partial_scheds.empty())
    {
        completion_counter = 1;
    }
    else
    {
        completion_counter = static_cast<int>(partial_scheds.size());
    }
    LOG_DEBUG("req ", req, ", set count ", completion_counter);

    req->set_counter(completion_counter);
    return req;
}

void mlsl_sched::add_barrier()
{
    if (!entries.empty())
    {
        if (add_mode == mlsl_sched_add_back)
            entries.back()->make_barrier();
        else if (add_mode == mlsl_sched_add_front)
            entries.front()->make_barrier();
        else
            MLSL_FATAL("unexpected mode ", add_mode);
    }
}

void mlsl_sched::sync_partial_scheds()
{
    if (partial_scheds.size() <= 1)
        return;

    auto sync_obj = std::make_shared<sync_object>(partial_scheds.size());
    for(auto& sched : partial_scheds)
    {
        entry_factory::make_sync_entry(sched.get(), sync_obj);
    }
}

void mlsl_sched::set_coll_attr(const mlsl_coll_attr_t* attr,
                               std::string match_id)
{
    MLSL_ASSERT(attr);
    coll_attr.prologue_fn = attr->prologue_fn;
    coll_attr.epilogue_fn = attr->epilogue_fn;
    coll_attr.reduction_fn = attr->reduction_fn;
    coll_attr.priority = attr->priority;
    coll_attr.synchronous = attr->synchronous;
    coll_attr.to_cache = attr->to_cache;
    coll_attr.match_id = std::move(match_id);
}

void mlsl_sched::dump_all() const
{
    for(const auto& sched : partial_scheds)
    {
        sched->dump("worker_sched");
    }

    dump("origin_sched");
}

void mlsl_sched::dump(const char *name) const
{
    if (!env_data.sched_dump)
        return;

    if(coll_param.ctype == mlsl_coll_internal)
    {
        return;
    }

    std::stringstream msg;
    mlsl_logger::format(msg, "\n--------------------------------\n");
    mlsl_logger::format(msg,
                        "sched: ", name, " ", this,
                        ", coll ", mlsl_coll_type_to_str(coll_param.ctype),
                        ", start_idx, ", start_idx,
                        ", num_entries ", entries.size(),
                        ", comm_id ", coll_param.comm->id(),
                        ", sched_id ", sched_id,
                        ", req ", req,
                        "\n");

    for (size_t i = 0; i < entries.size(); ++i)
    {
        entries[i]->dump(msg, i);
    }

#ifdef ENABLE_DEBUG
    mlsl_logger::format(msg, "life time [us] ", std::setw(5), std::setbase(10),
        std::chrono::duration_cast<std::chrono::microseconds>(exec_complete_time - exec_start_time).count(),
        "\n");
#endif
    mlsl_logger::format(msg, "--------------------------------\n");
    std::cout << msg.str();
}

void* mlsl_sched::alloc_buffer(size_t size)
{
    LOG_DEBUG("size ", size);
    MLSL_THROW_IF_NOT(size > 0, "incorrect buffer size");

    void* p = MLSL_CALLOC(size, "sched_buffer");
    memory.buf_list.emplace_back(p, size);
    return p;
}

void mlsl_sched::free_buffers()
{
    std::list<mlsl_sched_buffer_handler>::iterator it;
    for (it = memory.buf_list.begin(); it != memory.buf_list.end(); it++)
    {
        LOG_DEBUG("free ", it->ptr);
        MLSL_FREE(it->ptr);
    }
    memory.buf_list.clear();
}

mlsl_request* mlsl_sched::start_subsched(mlsl_sched* subsched)
{
    subsched->req->set_counter(1);
    subsched->sched_id = sched_id;
    subsched->reset();
    queue->add(subsched);
    return subsched->req;
}
