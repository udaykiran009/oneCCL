#include "sched/sched_queue.hpp"
#include "common/env/env.hpp"

void ccl_sched_bin::add(ccl_sched* sched)
{
    if (env_data.priority_mode != ccl_priority_none)
    {
        CCL_ASSERT(sched->coll_attr.priority == priority,
            "unexpected sched priority ", sched->coll_attr.priority,
            " expected ", priority);
    }
    CCL_ASSERT(sched);
    sched->bin = this;
    sched->queue = queue;
    sched_list.add(sched);

}
size_t ccl_sched_bin::erase(size_t idx, size_t& next_idx)
{
    ccl_sched* ret = nullptr;
    size_t size = 0;
    {
        std::lock_guard<sched_queue_lock_t> lock(sched_list.elem_guard);
        size_t size = sched_list.elems.size();
        CCL_ASSERT(idx < size);
        ret = sched_list.elems[idx];
        ret->bin = nullptr;
        size--;
        std::swap(sched_list.elems[size], sched_list.elems[idx]);
        sched_list.elems.resize(size);
        CCL_ASSERT(sched_list.elems.size() == (size));
        next_idx = idx;
    }
    return size;
}

size_t ccl_sched_bin::erase(size_t idx)
{
    size_t next_idx = 0;
    ccl_sched* sched = sched_list.remove(idx, next_idx);
    CCL_ASSERT(sched);
    sched->bin = nullptr;
    return next_idx;
}

ccl_sched_queue::ccl_sched_queue(std::vector<atl_comm_t*> comm_ctxs)
    : comm_ctxs(comm_ctxs)
{
    LOG_DEBUG("created sched_queue, comm_ctxs count ",  comm_ctxs.size(),
              ", comm_ctxs[0] ", comm_ctxs[0]);

    if (env_data.priority_mode != ccl_priority_none)
    {
        CCL_ASSERT(comm_ctxs.size() == CCL_PRIORITY_BUCKET_COUNT,
            "unexpected comm_cxt count ", comm_ctxs.size(), ", expected ",
            CCL_PRIORITY_BUCKET_COUNT);
    }
    else
        CCL_ASSERT(!comm_ctxs.empty());
}

ccl_sched_queue::~ccl_sched_queue()
{
    CCL_ASSERT(bins.empty(),
               "unexpected bins size ", bins.size(), ", expected 0");

    CCL_ASSERT(max_priority == 0,
               "unexpected max_priority ", max_priority, ", expected 0");

    CCL_ASSERT(!cached_max_priority_bin);
}

void ccl_sched_queue::add(ccl_sched* sched)
{
    if (sched->strict_start_order)
    {
        std::lock_guard<sched_queue_lock_t> lock{strict_order_queue_guard};
        strict_order_queue.push_back(sched);
        strict_order_queue_empty.clear();
    }
    else
    {
        add_internal(sched);
    }
}

void ccl_sched_queue::add_internal(ccl_sched* sched, bool need_to_lock /* = true */)
{
    size_t priority = sched->get_priority();
    if (env_data.priority_mode != ccl_priority_none)
    {
        if (sched->coll_param.ctype == ccl_coll_barrier)
        {
            priority = max_priority;
            sched->coll_attr.priority = priority;
        }
    }
    CCL_ASSERT(sched);

    LOG_DEBUG("sched ", sched, ", priority ", priority);

    ccl_sched_bin* bin = nullptr;

    // lock it, if requested
    std::unique_lock<sched_queue_lock_t> bins_lock(bins_guard, std::defer_lock);
    if (need_to_lock)
    {
        bins_lock.lock();
    }

    sched_bin_list_t::iterator it = bins.find(priority);
    if (it != bins.end())
    {
        bin = &(it->second);
        LOG_DEBUG("found bin ", bin);
        bin->add(sched);
    }
    else
    {
        atl_comm_t* comm_ctx = nullptr;
        if (env_data.priority_mode == ccl_priority_none)
            comm_ctx = comm_ctxs[0];
        else
        {
            size_t comm_idx = (priority / CCL_PRIORITY_BUCKET_SIZE) % CCL_PRIORITY_BUCKET_COUNT;
            comm_ctx = comm_ctxs[comm_idx];
            LOG_DEBUG("priority ", priority, ", comm_idx ", comm_idx);
        }

        // in-place construct priority bin with added sched
        auto emplace_result = bins.emplace(std::piecewise_construct,
                                           std::forward_as_tuple(priority),
                                           std::forward_as_tuple(this, comm_ctx, priority, sched));
        CCL_ASSERT(emplace_result.second);
        bin = &(emplace_result.first->second);
        if (priority >= max_priority)
        {
            max_priority = priority;
            cached_max_priority_bin = bin;
        }
        LOG_DEBUG("didn't find bin, emplaced new one, max_priority ", max_priority);
    }
    CCL_ASSERT(bin);
}

size_t ccl_sched_queue::erase(ccl_sched_bin* bin, size_t idx)
{
    CCL_ASSERT(bin);
    size_t bin_priority = bin->get_priority();

    LOG_DEBUG("queue ", this, ", bin ", bin);
    size_t next_idx = 0;

    // erase sched and check bin size after
    // no need to lock whole `bins` for single erase
    if (!bin->erase(idx, next_idx))
    {
        // 'bin 'looks like empty, we can erase it from 'bins'.
        // double check on bin.empty(), before remove it from whole table
        std::lock_guard<sched_queue_lock_t> lock{bins_guard};
        {
            // no need to lock 'bin' here, because all adding are under bins_guard protection
            if (bin->sched_list.elems.empty())
            {
                bins.erase(bin_priority);

                // change priority
                if (bins.empty())
                {
                    max_priority = 0;
                    cached_max_priority_bin = nullptr;
                }
                else if (bin_priority == max_priority)
                {
                    max_priority--;
                    sched_bin_list_t::iterator it;
                    while ((it = bins.find(max_priority)) == bins.end())
                    {
                        max_priority--;
                    }
                    cached_max_priority_bin = &(it->second);
                }
            } // or do nothing, because somebody added new element in bin why we getting a lock
        }
    }

    return next_idx;
}

void ccl_sched_queue::handle_strict_order_queue()
{
    if (!active_strict_order_queue.empty())
    {
        /* try to finish previous postponed operations */
        for (auto sched_it = active_strict_order_queue.begin();
             sched_it != active_strict_order_queue.end();
             sched_it++)
        {
            ccl_sched* sched = *sched_it;

            if (sched->is_executed())
            {
                CCL_ASSERT(sched->is_strict_order_satisfied());
                continue;
            }

            if (!sched->bin)
            {
                std::lock_guard<sched_queue_lock_t> lock{bins_guard};
                add_internal(sched, false);
            }

            sched->do_progress();

            if (!sched->is_strict_order_satisfied())
            {
                /*
                  we can't state that current operation is started with strict order
                  remove all previous operations from queue, as they were successfully started with strict order
                  and return to strict starting for current operation on the next call
                */
                std::vector<ccl_sched*> unhandled_scheds =
                    std::vector<ccl_sched*>(sched_it, active_strict_order_queue.end());
                active_strict_order_queue.swap(unhandled_scheds);
                return;
            }
        }
        active_strict_order_queue.clear();
    }
    else
    {
        if (!strict_order_queue_empty.test_and_set())
        {
            {
                std::lock_guard<sched_queue_lock_t> lock{strict_order_queue_guard};
                if (!strict_order_queue.empty())
                {
                    active_strict_order_queue.swap(strict_order_queue);
                }
                else
                {
                    CCL_ASSERT("unexpected path");
                }
            }
            handle_strict_order_queue();
        }
    }
}

ccl_sched_bin* ccl_sched_queue::peek()
{
    handle_strict_order_queue();
    return cached_max_priority_bin;
}

void ccl_sched_queue::clear()
{
    cached_max_priority_bin = nullptr;
    bins.clear();
    max_priority = 0;
}
