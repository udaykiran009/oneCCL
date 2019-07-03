#include "sched/sched_cache.hpp"

iccl_sched* iccl_sched_cache::find(iccl_sched_key& key)
{
    sched_table_t::iterator it;
    {
        std::lock_guard<sched_cache_lock_t> lock{guard};
        it = table.find(key);
    }

    if (it != table.end())
    {
        LOG_DEBUG("found sched in cache, ", it->second);
        return it->second;
    }
    else
    {
        LOG_DEBUG("didn't find sched in cache");
        return nullptr;
    }
}

void iccl_sched_cache::add(iccl_sched_key& key, iccl_sched* sched)
{
    {
        std::lock_guard<sched_cache_lock_t> lock{guard};
        auto emplace_result = table.emplace(key, sched);
        ICCL_ASSERT(emplace_result.second);
    }

    LOG_DEBUG("size ", table.size(), ",  bucket_count", table.bucket_count(), ", load_factor ", table.load_factor(),
              ", max_load_factor ", table.max_load_factor());
}

void iccl_sched_cache::remove_all()
{
    std::lock_guard<sched_cache_lock_t> lock{guard};
    for (auto it = table.begin(); it != table.end(); ++it)
    {
        iccl_sched* sched = it->second;
        ICCL_ASSERT(sched);
        LOG_DEBUG("remove sched ", sched, " from cache");
        delete sched;
    }
    table.clear();
}
