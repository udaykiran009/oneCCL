#include "sched/sched_queue.hpp"
#include "common/utils/utils.hpp"

void mlsl_sched_bin::add(mlsl_sched* sched)
{
    if (env_data.priority_mode != mlsl_priority_none)
    {
        MLSL_ASSERT_FMT(sched->coll_attr.priority == priority,
            "unexpected sched priority %zu, expected %zu",
            sched->coll_attr.priority, priority);
    }
    scheds.emplace_back(sched);
}

sched_list_t::iterator mlsl_sched_bin::erase(sched_list_t::iterator sched_it)
{
    (*sched_it)->bin = nullptr;
    sched_list_t::iterator next_sched_it = scheds.erase(sched_it);
    return next_sched_it;
}

mlsl_sched_queue::mlsl_sched_queue(std::vector<atl_comm_t*> comm_ctxs)
    : comm_ctxs(comm_ctxs)
{
    MLSL_LOG(DEBUG, "created sched_queue, comm_ctxs count %zu", comm_ctxs.size());

    if (env_data.priority_mode != mlsl_priority_none)
    {
        MLSL_ASSERT_FMT(comm_ctxs.size() == MLSL_PRIORITY_BUCKET_COUNT,
            "unexpected comm_cxt count %zu, expected %d",
            comm_ctxs.size(), MLSL_PRIORITY_BUCKET_COUNT);
    }
    else
        MLSL_ASSERT(!comm_ctxs.empty());
}

mlsl_sched_queue::~mlsl_sched_queue()
{
    MLSL_ASSERT_FMT(bins.empty(),
        "unexpected bins size %zu, expected 0",
        bins.size());

    MLSL_ASSERT_FMT(max_priority == 0,
        "unexpected max_priority %zu, expected 0",
        max_priority);

    MLSL_ASSERT(!cached_max_priority_bin);
}

void mlsl_sched_queue::add(mlsl_sched* sched, size_t priority)
{
    if (env_data.priority_mode != mlsl_priority_none)
    {
        if (sched->coll_param.ctype == mlsl_coll_barrier)
        {
            priority = max_priority;
            sched->coll_attr.priority = priority;
        }
    }

    std::lock_guard<std::mutex> lock{guard};

    MLSL_LOG(DEBUG, "sched %p, priority %zu", sched, priority);

    mlsl_sched_bin* bin = nullptr;
    sched_bin_list_t::iterator it = bins.find(priority);
    if (it != bins.end())
    {
        it->second.add(sched);
        bin = &(it->second);
        MLSL_LOG(DEBUG, "found bin %p", bin);
    }
    else
    {
        atl_comm_t* comm_ctx = nullptr;
        if (env_data.priority_mode == mlsl_priority_none)
            comm_ctx = comm_ctxs[0];
        else
        {
            size_t comm_idx = (priority / MLSL_PRIORITY_BUCKET_SIZE) % MLSL_PRIORITY_BUCKET_COUNT;
            comm_ctx = comm_ctxs[comm_idx];
            MLSL_LOG(DEBUG, "priority %zu, comm_idx %zu", priority, comm_idx);
        }

        auto emplace_result = bins.emplace(priority, mlsl_sched_bin{this, comm_ctx, priority});
        MLSL_ASSERT(emplace_result.second);
        bin = &(emplace_result.first->second);
        bin->add(sched);
        if (priority >= max_priority)
        {
            max_priority = priority;
            cached_max_priority_bin = bin;
        }
        MLSL_LOG(DEBUG, "didn't find bin, emplaced new one, max_priority %zu", max_priority);
    }
    MLSL_ASSERT(bin);

    sched->bin = bin;
    sched->queue = this;
}

sched_list_t::iterator mlsl_sched_queue::erase(sched_list_t::iterator sched_it)
{
    mlsl_sched* sched = *sched_it;
    MLSL_ASSERT(sched);

    mlsl_sched_bin* bin = sched->bin;
    MLSL_ASSERT(bin);

    MLSL_LOG(DEBUG, "queue %p, bin %p, sched %p", this, bin, sched);
    size_t bin_priority = bin->get_priority();

    std::lock_guard<std::mutex> lock{guard};
    sched_list_t::iterator next_sched = bin->erase(sched_it);
    size_t bin_size = bin->size();
    sched_list_t::iterator last_sched = std::end(bin->get_scheds());
    if (bin_size == 0)
    {
        bins.erase(bin_priority);
        sched->bin = nullptr;
    }

    if (bins.empty())
    {
        max_priority = 0;
        cached_max_priority_bin = nullptr;
    }
    else if ((bin_priority == max_priority) && (bin_size == 0))
    {
        max_priority--;
        sched_bin_list_t::iterator it;
        while ((it = bins.find(max_priority)) == bins.end())
        {
            max_priority--;
        }
        cached_max_priority_bin = &(it->second);
    }
    return next_sched;
}

mlsl_sched_bin* mlsl_sched_queue::peek(size_t& bin_size)
{
    std::lock_guard<std::mutex> lock{guard};
    bin_size = (cached_max_priority_bin) ? cached_max_priority_bin->size() : 0;
    return cached_max_priority_bin;
}
