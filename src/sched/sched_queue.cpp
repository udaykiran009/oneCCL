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
size_t ccl_sched_bin::erase(size_t idx, size_t &nextIdx)
{
    ccl_sched* ret = nullptr;
    size_t size = 0;
    {
        std::lock_guard<sched_queue_lock_t> lock(sched_list.elemGuard);
        size_t size = sched_list.elems.size();
        CCL_ASSERT(idx < size);
        ret = sched_list.elems[idx];
        ret->bin = nullptr;
        size--;
        std::swap(sched_list.elems[size], sched_list.elems[idx]);
        sched_list.elems.resize(size);
        CCL_ASSERT(sched_list.elems.size() == (size));
        nextIdx = idx;
    }
    return size;
}

size_t ccl_sched_bin::erase(size_t idx)
{
    size_t nextId = 0;
    ccl_sched* sched = sched_list.remove(idx, nextId);
    CCL_ASSERT(sched);
    sched->bin = nullptr;
    return nextId;
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
        // use postponed queue here
        std::lock_guard<sched_queue_lock_t> lock{postponed_queue_guard};
        postponed_queue.push_back(sched);
        postponed_queue_empty.clear();
    }
    else
    {
        add_internal(sched);
    }
}

void ccl_sched_queue::add_internal(ccl_sched* sched, bool need_to_lock /* = true*/)
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

ccl_sched_bin* ccl_sched_queue::peek()
{
    // check postponed queue emptyflag at first
    // no need to lock whole `postponed_queue_guard` here
    if (postponed_queue_empty.test_and_set())
    {
        // no need to to iterates over postponed, just return max bin
        return cached_max_priority_bin;
    }

    // postponed queue is not empty in this case, process it at first
    std::vector<ccl_sched*> current_processing_queue;
    {
        std::lock_guard<sched_queue_lock_t> lock{postponed_queue_guard};
        if (!postponed_queue.empty())
        {
            current_processing_queue.swap(postponed_queue);
        }
    }

    // queue into priority bins
    ccl_sched_bin* result = nullptr;
    {
        std::lock_guard<sched_queue_lock_t> lock{bins_guard};
        for (auto sched : current_processing_queue)
        {
            add_internal(sched, false);
        }
        result = cached_max_priority_bin;
    }

    // process scheds without locks
    for (auto sched : current_processing_queue)
    {
        ccl_sched_progress(sched);
    }
    current_processing_queue.clear();

    return result;
}
