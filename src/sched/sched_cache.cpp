#include "sched/sched_cache.hpp"

ccl_sched* ccl_sched_cache::find(ccl_sched_key& key)
{
    sched_table_t::iterator it;
    {
        std::lock_guard<sched_cache_lock_t> lock{guard};
        it = table.find(key);
    }

    if (it != table.end())
    {
        LOG_DEBUG("found sched in cache, ", it->second);
        ccl_sched* sched = it->second;
        if (!env_data.full_cache_key)
        {
            LOG_DEBUG("do check for found sched");
            CCL_ASSERT(sched->coll_attr.prologue_fn == key.prologue_fn, "prologue_fn");
            CCL_ASSERT(sched->coll_attr.epilogue_fn == key.epilogue_fn, "epilogue_fn");
            CCL_ASSERT(sched->coll_attr.reduction_fn == key.reduction_fn, "reduction_fn");
            CCL_ASSERT(sched->coll_param.ctype == key.ctype, "ctype");
            CCL_ASSERT(sched->coll_param.dtype->type == key.dtype, "dtype");
            CCL_ASSERT(sched->coll_param.comm == key.comm, "comm");

            if (sched->coll_param.ctype == ccl_coll_allgatherv)
                CCL_ASSERT(sched->coll_param.send_count == key.count1, "count");
            else
                CCL_ASSERT(sched->coll_param.count == key.count1, "count");

            if (sched->coll_param.ctype == ccl_coll_bcast || sched->coll_param.ctype == ccl_coll_reduce)
            {
                CCL_ASSERT(sched->coll_param.root == key.root, "root");
            }

            if (sched->coll_param.ctype == ccl_coll_allreduce ||
                sched->coll_param.ctype == ccl_coll_reduce ||
                sched->coll_param.ctype == ccl_coll_sparse_allreduce)
            {
                CCL_ASSERT(sched->coll_param.reduction == key.reduction, "reduction");
            }
        }
        return sched;
    }
    else
    {
        LOG_DEBUG("didn't find sched in cache");
        return nullptr;
    }
}

void ccl_sched_cache::add(ccl_sched_key& key, ccl_sched* sched)
{
    {
        std::lock_guard<sched_cache_lock_t> lock{guard};
        auto emplace_result = table.emplace(key, sched);
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
        ccl_sched* sched = it->second;
        CCL_ASSERT(sched);
        LOG_DEBUG("remove sched ", sched, " from cache");
        delete sched;
    }
    table.clear();
}
