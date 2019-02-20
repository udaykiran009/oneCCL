#include "sched/sched_queue.hpp"
#include "common/utils/utils.hpp"


mlsl_sched_queue::mlsl_sched_queue(size_t capacity, atl_comm_t **comm_ctxs)
    : bins(capacity), max_bins(capacity)
{
    MLSL_ASSERTP(max_bins <= MLSL_SCHED_QUEUE_MAX_BINS);
    mlsl_fastlock_init(&lock);

    for (size_t idx = 0; idx < max_bins; idx++)
    {
        bins[idx].queue = this;
        bins[idx].comm_ctx = comm_ctxs[idx];
        MLSL_LOG(DEBUG, "comm_ctxs[%zu]: %p", idx, comm_ctxs[idx]);
    }
    MLSL_LOG(INFO, "used_bins %zu max_prio %zu", used_bins, max_priority);
}

mlsl_sched_queue::~mlsl_sched_queue()
{
    mlsl_fastlock_destroy(&lock);
    //todo: MLSL_ASSERT throws an exception which is unacceptable in destructors
    //need to create another macro to handle errors via e.g. exit()
    assert(used_bins == 0);
    assert(max_priority == 0);
}

void mlsl_sched_queue::add(mlsl_sched* sched, size_t priority)
{
    mlsl_fastlock_acquire(&lock);
    mlsl_sched_queue_bin *bin = &(bins[priority % max_bins]);
    if (bin->elems.empty())
    {
        MLSL_ASSERT(bin->priority == 0);
        bin->priority = priority;
        ++used_bins;
    }

    bin->elems.push_back(sched);
    sched->bin = bin;
    max_priority = std::max(max_priority, priority);
    mlsl_fastlock_release(&lock);
}

void mlsl_sched_queue::erase(mlsl_sched_queue_bin* bin, mlsl_sched* sched)
{
    MLSL_LOG(DEBUG, "queue %p, bin %p, sched %p, elems count %zu",
             this, bin, sched, bin->elems.size());

    mlsl_fastlock_acquire(&lock);
    bin->elems.remove(sched);
    if (bin->elems.empty())
    {
        update_priority_on_erase();
        bin->priority = 0;
    }
    mlsl_fastlock_release(&lock);
}

std::list<mlsl_sched*>::iterator mlsl_sched_queue::erase(mlsl_sched_queue_bin* bin, std::list<mlsl_sched*>::iterator it)
{
    MLSL_LOG(DEBUG, "queue %p, bin %p, sched %p, elems count %zu",
             this, bin, *it, bin->elems.size());

    mlsl_fastlock_acquire(&lock);
    auto next_it = bin->elems.erase(it);
    if (bin->elems.empty())
    {
        update_priority_on_erase();
        bin->priority = 0;
    }
    mlsl_fastlock_release(&lock);
    return next_it;
}

void mlsl_sched_queue::update_priority_on_erase()
{
    used_bins--;
    if (used_bins == 0)
    {
        max_priority = 0;
    }
    else
    {
        //bins are arranged by (maximum supported priority) % max_bins
        size_t bin_idx = max_priority % max_bins;
        while (bins[bin_idx].elems.empty())
        {
            bin_idx = (bin_idx - 1 + max_bins) % max_bins;
        }
        max_priority = bins[bin_idx].priority;
    }
}

mlsl_sched_queue_bin* mlsl_sched_queue::peek(size_t& count)
{
    mlsl_fastlock_acquire(&lock);
    mlsl_sched_queue_bin* result = nullptr;
    if (used_bins > 0)
    {
        result = &(bins[max_priority % max_bins]);
        count = result->elems.size();
        MLSL_ASSERT(count > 0);
    }
    else
    {
        count = 0;
    }
    mlsl_fastlock_release(&lock);

    return result;
}
