#include "common/env/env.hpp"
#include "sched/cache/cache.hpp"

ccl_master_sched* ccl_sched_cache::find_unsafe(const ccl_sched_key& key) const
{
    ccl_master_sched* sched = nullptr;
    {
        auto it = table.find(key);
        if (it != table.end())
        {
            sched = it->second;
        }
    }
    return sched;
}

void ccl_sched_cache::recache(const ccl_sched_key& old_key, ccl_sched_key&& new_key)
{
    {
        std::lock_guard<sched_cache_lock_t> lock{guard};
        auto it = table.find(old_key);
        if (it == table.end())
        {
            std::string error_message = "old_key wasn't found";
            CCL_ASSERT(false, error_message, old_key.match_id);
            throw ccl::ccl_error(error_message + old_key.match_id);
        }
        ccl_master_sched* sched = it->second;
        table.erase(it);
        auto emplace_result = table.emplace(std::move(new_key), sched);
        CCL_THROW_IF_NOT(emplace_result.second);
    }
}

void ccl_sched_cache::release(ccl_master_sched* sched)
{
    reference_counter--;
    LOG_TRACE("reference_counter=",  reference_counter);
}

bool ccl_sched_cache::try_flush()
{
    if (!env_data.enable_cache_flush)
        return true;

    std::lock_guard<sched_cache_lock_t> lock{guard};

    if (reference_counter == 0)
    {
        for (auto it = table.begin(); it != table.end(); ++it)
        {
            ccl_master_sched* sched = it->second;
            CCL_ASSERT(sched);
            LOG_DEBUG("remove sched ", sched, " from cache");
            delete sched;
        }
        table.clear();
        return true;
    }
    else
    {
        return false;
    }
}
