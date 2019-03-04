#include "sched/sched_cache.hpp"

mlsl_sched* mlsl_sched_cache::find(mlsl_sched_key& key)
{
    sched_table_t::iterator it;
    {
        std::lock_guard<sched_cache_lock_t> lock{guard};
        it = table.find(key);
    }

    if (it != table.end())
    {
        MLSL_LOG(DEBUG, "found sched in cache, %p", it->second);
        return it->second;
    }
    else
    {
        MLSL_LOG(DEBUG, "didn't find sched in cache");
        return nullptr;
    }
}

void mlsl_sched_cache::add(mlsl_sched_key& key, mlsl_sched* sched)
{
    {
        std::lock_guard<sched_cache_lock_t> lock{guard};
        auto emplace_result = table.emplace(key, sched);
        MLSL_ASSERT(emplace_result.second);
    }

    MLSL_LOG(DEBUG, "size %zu, bucket_count %zu, load_factor %f, max_load_factor %f",
             table.size(), table.bucket_count(), table.load_factor(), table.max_load_factor());
}

void mlsl_sched_cache::remove_all()
{
    std::lock_guard<sched_cache_lock_t> lock{guard};
    for (auto it = table.begin(); it != table.end(); ++it)
    {
        mlsl_sched* sched = it->second;
        MLSL_ASSERT(sched);
        MLSL_LOG(DEBUG, "remove sched %p from cache", sched);
        delete sched;
    }
    table.clear();
}
