#include "common/env/env.hpp"
#include "common/global/global.hpp"
#include "exec/exec.hpp"
#include "exec/thread/worker.hpp"

#define CCL_WORKER_CHECK_STOP_ITERS (32768)
#define CCL_WORKER_CHECK_UPDATE_ITERS (16384)
#define CCL_WORKER_PROCESS_ALL_ITERS  (4096)

static void* ccl_worker_func(void* args);

ccl_worker::ccl_worker(size_t idx,
                       std::unique_ptr<ccl_sched_queue> queue)
    : ccl_base_thread(idx, ccl_worker_func),
      should_lock(false), is_locked(false),
      strict_sched_queue(std::unique_ptr<ccl_strict_sched_queue>(new ccl_strict_sched_queue())),
      sched_queue(std::move(queue))
{ }

void ccl_worker::add(ccl_sched* sched)
{
    LOG_DEBUG("add sched ", sched, ", type ",
               ccl_coll_type_to_str(sched->coll_param.ctype));

    CCL_ASSERT(sched);
    CCL_ASSERT(!sched->bin);
    CCL_ASSERT(sched->get_in_bin_status() != ccl_sched_in_bin_added);

    if (sched->strict_start_order)
    {
        /* to keep valid non-completed req until safe releasing */
        sched->req->increase_counter(1);
        strict_sched_queue->add(sched);
    }
    else
    {
        sched_queue->add(sched);
    }
}

ccl_status_t ccl_worker::do_work(size_t& processed_count)
{
    do_work_counter++;

    auto ret = process_strict_sched_queue();
    if (ret != ccl_status_success)
        return ret;

    ret = process_sched_queue(processed_count,
                              (do_work_counter % CCL_WORKER_PROCESS_ALL_ITERS) ? false : true);
    if (ret != ccl_status_success)
        return ret;

#ifdef ENABLE_DEBUG
    if (processed_count == 0 && (do_work_counter % CCL_WORKER_PROCESS_ALL_ITERS * 1024) == 0)
    {
        //sched_queue->dump(std::cout);
    }
#endif

    return ccl_status_success;
}

ccl_status_t ccl_worker::process_strict_sched_queue()
{
    auto& queue = strict_sched_queue->peek();
    if (queue.empty())
        return ccl_status_success;

    size_t erased_scheds = 0;

    /* try to finish previous postponed operations */
    for (auto sched_it = queue.begin(); sched_it != queue.end(); sched_it++)
    {
        ccl_sched* sched = *sched_it;

        if (sched->get_in_bin_status() == ccl_sched_in_bin_erased)
        {
            CCL_ASSERT(!sched->bin);
            erased_scheds++;

            /* only single sched in active strict queue can be erased since previous call */
            CCL_ASSERT(erased_scheds == 1);

            /* now it is safe to release this sched */
            sched->req->complete();
            continue;
        }

        if (sched->get_in_bin_status() == ccl_sched_in_bin_none)
        {
            CCL_ASSERT(!sched->bin, "unexpected bin ", sched->bin);
            /* here we add sched from strict_queue to regular queue for real execution */
            LOG_DEBUG("add sched ", sched, " from strict_queue to exec_queue, req ", sched->req);
            sched_queue->add(sched);
        }

        CCL_ASSERT(sched->get_in_bin_status() == ccl_sched_in_bin_added,
                   "sched ", sched, " unexpected in_bin_status ", sched->get_in_bin_status());

        sched->do_progress();

        if (!sched->is_strict_order_satisfied())
        {
            /*
                we can't state that current operation is started with strict order
                remove all previous operations from queue, as they were successfully started with strict order
                and return to strict starting for current operation on the next call
            */
            std::vector<ccl_sched*>(sched_it, queue.end()).swap(queue);
            return ccl_status_success;
        }
        else
        {
            /* now it is safe to release this sched */
            sched->req->complete();
        }
    }

    queue.clear();

    return ccl_status_success;
}

ccl_status_t ccl_worker::process_sched_queue(size_t& completed_sched_count, bool process_all)
{
    completed_sched_count = 0;
    if (process_all)
    {
        auto bins = sched_queue->peek_all();

        if (bins.empty())
            return ccl_status_success;

        size_t completed_sched_count_local = 0;
        for (auto& bin : bins)
        {
            process_sched_bin(bin, completed_sched_count_local);
            completed_sched_count += completed_sched_count_local;
        }

        if (completed_sched_count)
            LOG_DEBUG("process_all, completed_sched_count ", completed_sched_count);

        return ccl_status_success;
    }
    else
    {
        ccl_sched_bin* bin = sched_queue->peek();
        if (!bin)
            return ccl_status_success;
        return process_sched_bin(bin, completed_sched_count);
    }
}

ccl_status_t ccl_worker::process_sched_bin(ccl_sched_bin* bin, size_t& completed_sched_count)
{
    CCL_ASSERT(bin);
    completed_sched_count = 0;

    size_t bin_size = bin->size();
    CCL_ASSERT(bin_size > 0 );

    LOG_TRACE("bin ", bin, ", sched_count ", bin_size);

    /* ensure communication progress */
    atl_status_t atl_status = atl_ep_poll(bin->get_atl_ep());
    if (global_data.is_ft_enabled)
    {
        if (atl_status != ATL_STATUS_SUCCESS)
            return ccl_status_blocked_due_to_resize;
    }
    else
    {
        CCL_THROW_IF_NOT(atl_status == ATL_STATUS_SUCCESS, "bad status ", atl_status);
    }

    // iterate through the scheds stored in the bin
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
            CCL_ASSERT(!sched->bin);
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
    }

    return ccl_status_success;
}

void ccl_worker::clear_queue()
{
    strict_sched_queue->clear();
    sched_queue->clear();
}

static void* ccl_worker_func(void* args)
{
    auto worker = static_cast<ccl_worker*>(args);
    LOG_DEBUG("worker_idx ", worker->get_idx());

    size_t iter_count = 0;
    size_t processed_count = 0;
    size_t spin_count = env_data.spin_count;
    size_t max_spin_count = env_data.spin_count;
    ccl_status_t ret;

    global_data.is_worker_thread = true;
    worker->started = true;

    do
    {
        try
        {
            // thread is non-interruptible from this point
            ret = worker->do_work(processed_count);

            if (global_data.is_ft_enabled &&
                unlikely(ret == ccl_status_blocked_due_to_resize || iter_count % CCL_WORKER_CHECK_UPDATE_ITERS == 0))
            {
                if (worker->should_lock.load(std::memory_order_acquire))
                {
                    worker->clear_queue();
                    worker->is_locked = true;
                    while (worker->should_lock.load(std::memory_order_relaxed))
                    {
                        ccl_yield(env_data.yield_type);
                    }
                    worker->is_locked = false;
                }
            }

        }
        catch (ccl::ccl_error& ccl_e)
        {
            CCL_FATAL("worker ", worker->get_idx(), " caught internal exception: ", ccl_e.what());
        }
        catch (std::exception& e)
        {
            CCL_FATAL("worker ", worker->get_idx(), " caught exception: ", e.what());
        }
        catch (...)
        {
            CCL_FATAL("worker ", worker->get_idx(), " caught general exception");
        }

        iter_count++;
        if ((iter_count % CCL_WORKER_CHECK_STOP_ITERS) == 0)
        {
            if (worker->should_stop.load(std::memory_order_acquire))
                break;
        }

        if (processed_count == 0)
        {
            spin_count--;
            if (!spin_count)
            {
                ccl_yield(env_data.yield_type);
                spin_count = 1;
            }
        }
        else
        {
            spin_count = max_spin_count;
        }
    } while (true);

    worker->started = false;

    return nullptr;
}
