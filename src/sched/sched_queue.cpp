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
    sched_list.add(sched);
}

size_t ccl_sched_bin::erase(size_t idx)
{
    ccl_sched* sched = sched_list.get(idx);
    CCL_ASSERT(sched);
    sched->bin = nullptr;
    return sched_list.erase(idx);
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
    std::lock_guard<sched_queue_lock_t> lock{guard};

    if (sched->strict_start_order)
        postponed_queue.push_back(sched);
    else
        add_internal(sched);
}

void ccl_sched_queue::add_internal(ccl_sched* sched)
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
    sched_bin_list_t::iterator it = bins.find(priority);
    if (it != bins.end())
    {
        it->second.add(sched);
        bin = &(it->second);
        LOG_DEBUG("found bin ", bin);
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

        auto emplace_result = bins.emplace(priority, ccl_sched_bin{this, comm_ctx, priority});
        CCL_ASSERT(emplace_result.second);
        bin = &(emplace_result.first->second);
        bin->add(sched);
        if (priority >= max_priority)
        {
            max_priority = priority;
            cached_max_priority_bin = bin;
        }
        LOG_DEBUG("didn't find bin, emplaced new one, max_priority ", max_priority);
    }
    CCL_ASSERT(bin);

    sched->bin = bin;
    sched->queue = this;
}

size_t ccl_sched_queue::erase(ccl_sched_bin* bin, size_t idx)
{
    CCL_ASSERT(bin);
    ccl_sched* sched = bin->get(idx);
    CCL_ASSERT(sched);

    LOG_DEBUG("queue ", this, ", bin ", bin, ", sched ", sched);
    size_t bin_priority = bin->get_priority();

    std::lock_guard<sched_queue_lock_t> lock{guard};
    size_t next_idx = bin->erase(idx);
    size_t bin_size = bin->size();

    if (bin_size == 0)
    {
        bins.erase(bin_priority);
        LOG_DEBUG("erase bin ", bin);
    }

    if (bins.empty())
    {
        max_priority = 0;
        cached_max_priority_bin = nullptr;
    }
    else if ((bin_priority == max_priority) && !bin_size)
    {
        max_priority--;
        sched_bin_list_t::iterator it;
        while ((it = bins.find(max_priority)) == bins.end())
        {
            max_priority--;
        }
        cached_max_priority_bin = &(it->second);
    }

    return next_idx;
}

ccl_sched_bin* ccl_sched_queue::peek(size_t& bin_size)
{
    std::lock_guard<sched_queue_lock_t> lock{guard};

    while (!postponed_queue.empty())
    {
        ccl_sched* sched = postponed_queue.front();
        add_internal(sched);
        ccl_sched_progress(sched);
        postponed_queue.pop_front();
    }
    
    bin_size = (cached_max_priority_bin) ? cached_max_priority_bin->size() : 0;
    return cached_max_priority_bin;
}
