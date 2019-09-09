#include "common/env/env.hpp"
#include "sched/cache/cache.hpp"

ccl_master_sched* ccl_sched_cache::find(ccl_sched_key& key)
{
    ccl_master_sched* sched = nullptr;
    {
        std::lock_guard<sched_cache_lock_t> lock{guard};
        sched_table_t::iterator it = table.find(key);
        if (it != table.end())
        {
            sched = it->second;
        }
    }

    if (sched)
    {
        LOG_DEBUG("found sched in cache, ",sched);
        if (env_data.cache_key_type != ccl_cache_key_full)
        {
            LOG_DEBUG("do check for found sched");
            CCL_ASSERT(key.check(sched->coll_param, sched->coll_attr));
        }
        return sched;
    }
    else
    {
        LOG_DEBUG("didn't find sched in cache");
        return nullptr;
    }
}

void ccl_sched_cache::add(ccl_sched_key&& key, ccl_master_sched* sched)
{
    {
        std::lock_guard<sched_cache_lock_t> lock{guard};
        auto emplace_result = table.emplace(std::move(key), sched);
        CCL_ASSERT(emplace_result.second);
    }

    LOG_DEBUG("size ", table.size(),
              ", bucket_count ", table.bucket_count(),
              ", load_factor ", table.load_factor(),
              ", max_load_factor ", table.max_load_factor());
}

void ccl_sched_cache::remove_all()
{
    std::lock_guard<sched_cache_lock_t> lock{guard};
    for (auto it = table.begin(); it != table.end(); ++it)
    {
        ccl_master_sched* sched = it->second;
        CCL_ASSERT(sched);
        LOG_DEBUG("remove sched ", sched, " from cache");
        delete sched;
    }
    table.clear();
}
