#include "sched/sched.hpp"
#include "sched/extra_sched.hpp"
#include "sched/sched_queue.hpp"
#include "sched/sync_object.hpp"
#include "sched/entry_factory.hpp"
#include "common/global/global.hpp"
#include "parallelizer/parallelizer.hpp"

ccl_sched::~ccl_sched()
{
    if (finalize_fn)
    {
        finalize_fn(this, finalize_fn_ctx);
    }

    if (!memory.mr_list.empty())
    {
        /* perform deregistration in worker thread */
        {
            ccl_coll_param cparam{};
            cparam.ctype = ccl_coll_internal;
            cparam.comm = coll_param.comm;
            std::unique_ptr<ccl_extra_sched> dereg_sched(new ccl_extra_sched(cparam, sched_id));
            entry_factory::make_entry<deregister_entry>(dereg_sched.get(), memory.mr_list);
            if (global_data.is_worker_thread || !env_data.worker_offload)
            {
                dereg_sched->start_entries();
            }
            else
            {
                //release ownership, because ccl_wait_impl use delete inside
                ccl_wait_impl<ccl_extra_sched>(global_data.executor.get(), start_subsched(dereg_sched.release()));
            }
        }
        if(!memory.mr_list.empty())
        {
            LOG_ERROR("memory list is not empty");
        }
        CCL_ASSERT(memory.mr_list.empty());
    }

    free_buffers();
}

/* Posts or performs any NOT_STARTED operations in the given schedule that are
 * permitted to be started.  That is, this routine will respect schedule
 * barriers appropriately. */
void ccl_sched::start_entries()
{
    for (size_t i = start_idx; i < entries.size(); ++i)
    {
        auto& entry = entries[i];

        if (entry->get_status() == ccl_sched_entry_status_not_started)
        {
            LOG_DEBUG("starting entry ", entry->name(), " [", i, "/", entries.size(), "]");
            entry->start();
        }

        /* _start_entry may have completed the operation, but won't update start_idx */
        if (i == start_idx && entry->get_status() >= ccl_sched_entry_status_complete)
        {
            ++start_idx;   /* this is valid even for barrier entries */
            LOG_DEBUG("completed ", entry->name(), entry->is_barrier() ? " barrier" : "",
                " entry [", i, "/", entries.size(), "], shift start_idx to ", start_idx, ", sched ", this);
        }

        /* watch the indexing, start_idx might have been incremented above, so
         * ||-short-circuit matters here */
        if (entry->is_barrier() && (entry->get_status() < ccl_sched_entry_status_complete || (start_idx != i + 1)))
        {
            /* we've hit a barrier but outstanding operations before this
             * barrier remain, so we cannot proceed past the barrier */
            break;
        }
    }
}

ccl_status_t ccl_bin_progress(ccl_sched_bin* bin,
                              size_t& completed_sched_count)
{
    CCL_ASSERT(bin);

    ccl_status_t status = ccl_status_success;
    size_t sched_count = 0;
    auto sched_queue = bin->get_queue();
    size_t bin_size = bin->size();
    CCL_ASSERT(bin_size > 0 );

    LOG_TRACE("bin ", bin, ", sched_count ", bin_size);

    /* ensure communication progress */
    atl_status_t atl_status = atl_comm_poll(bin->get_comm_ctx());
    CCL_THROW_IF_NOT(atl_status == atl_status_success, "bad status ", atl_status);

    // iterate through the scheds store in the bin
    completed_sched_count = 0;
    for (size_t sched_idx = 0; sched_idx < bin_size; )
    {
        ccl_sched* sched = bin->get(sched_idx);
        CCL_ASSERT(sched && bin == sched->bin);

        sched->do_progress();

        if (sched->start_idx == sched->entries.size())
        {
            // the last entry in the schedule has been completed, clean up the schedule and complete its request
            LOG_DEBUG("complete and dequeue: sched ", sched,
                ", coll ", ccl_coll_type_to_str(sched->coll_param.ctype),
                ", req ", sched->req,
                ", entry_count ", sched->entries.size());

            // remove completed schedule from the bin
            sched_queue->erase(bin, sched_idx);
            bin_size--;
            LOG_DEBUG("completing request ", sched->req);
            sched->complete();
            ++completed_sched_count;
        }
        else
        {
            // this schedule is not completed yet, switch to the next sched in bin scheds list
            // progression of unfinished schedules will be continued in the next call of @ref ccl_bin_progress
            ++sched_idx;
        }
        sched_count++;
    }

    return status;
}

void ccl_sched::do_progress()
{
    if (first_progress)
    {
        // perform initial progress, iterate through schedule entries
        // some entries are running in 'synchronous' way, they are completed right after execution
        // other entries are running in 'asynchronous' way and may not be completed right after execution
        // entry N+1 may be started right after entry N even if entry N has not been completed
        // if entry N+1 (and all subsequent entries) depends on entry N, then entry N is marked as a barrier and
        //     entry N+1 (and all subsequent) won't be started until entry N is completed
        /* TODO: do we need special handling for first_progress ? */
        LOG_DEBUG("initial start_entries for sched ", this);
        start_entries();
        first_progress = false;
    }

    // continue iteration of already started schedule. @b start_idx is an index of the first non-started entry
    for (auto entry_idx = start_idx; entry_idx < entries.size(); ++entry_idx)
    {
        auto& entry = entries[entry_idx];

        // check for completion of 'asynchronous' entries
        entry->update();

        if (entry_idx == start_idx && entry->get_status() >= ccl_sched_entry_status_complete)
        {
            // the entry has been completed, increment start_idx
            ++start_idx;
            LOG_DEBUG("completed ", entry->name(), entry->is_barrier() ? " barrier" : "",
                      " entry [", entry_idx, "/", entries.size(), "], shift start_idx to ", start_idx,
                      ", sched ", this);
            if (entry->is_barrier())
            {
                // that entry was marked as a barrier, run the rest entries (if any) which depend on it
                start_entries();
            }
        }
        else if (entry->is_barrier() && entry->get_status() < ccl_sched_entry_status_complete)
        {
            // the entry has not been completed yet. It is marked as a barrier, don't process anything after it
            break;
        }
    }
}

bool ccl_sched::is_strict_order_satisfied()
{
    CCL_ASSERT(strict_start_order);
    return std::all_of(entries.begin(), entries.end(), [](const sched_entry_ptr& e)
        {
            return e->is_strict_order_satisfied();
        });
}

bool ccl_sched::is_executed()
{
    return (start_idx == entries.size());
}

void ccl_sched::complete()
{
#ifdef ENABLE_TIMERS
    exec_complete_time = timer_type::now();
    if (env_data.sched_dump)
    {
        dump(std::cout);
    }
#endif
    CCL_ASSERT(req, "ccl_sched must have req");
    req->complete();
}

void ccl_sched::renew(bool need_update_id/* = false*/)
{
    if (need_update_id)
    {
        update_id();
    }
#ifdef ENABLE_TIMERS
    exec_start_time = timer_type::now();
    exec_complete_time = exec_start_time;
#endif
    start_idx = 0;
    first_progress = true;
    for (size_t idx = 0; idx < entries.size(); idx++)
    {
        entries[idx].get()->reset(idx);
    }
}

void ccl_sched::add_barrier()
{
    if (!entries.empty())
    {
        if (add_mode == ccl_sched_add_back)
            entries.back()->make_barrier();
        else if (add_mode == ccl_sched_add_front)
            entries.front()->make_barrier();
        else
            CCL_FATAL("unexpected add_mode ", add_mode);
    }
}

ccl_request* ccl_sched::start_subsched(ccl_extra_sched* subsched)
{
    subsched->set_counter(1);
    subsched->coll_attr.priority = coll_attr.priority;
    subsched->renew();
    queue->add(subsched);
    subsched->dump(std::cout);
    return subsched->req;
}

void ccl_sched::dump(std::ostream &out) const
{
    if (!env_data.sched_dump)
    {
        return;
    }

    ccl_sched_base::dump(out, class_name());
    ccl_logger::format(out, ", start_idx: ", start_idx,
                       ", num_entries: ", entries.size(),
                       ", priority: ", get_priority(),
                       "\n");

    std::stringstream msg;
    for (size_t i = 0; i < entries.size(); ++i)
    {
        entries[i]->dump(msg, i);
    }
    out << msg.str();
    ccl_logger::format(out, "--------------------------------\n");
}

