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
        iccl_sched* sched = it->second;
        if (!env_data.full_cache_key)
        {
            LOG_DEBUG("do check for found sched");
            ICCL_ASSERT(sched->coll_attr.prologue_fn == key.prologue_fn, "prologue_fn");
            ICCL_ASSERT(sched->coll_attr.epilogue_fn == key.epilogue_fn, "epilogue_fn");
            ICCL_ASSERT(sched->coll_attr.reduction_fn == key.reduction_fn, "reduction_fn");
            ICCL_ASSERT(sched->coll_attr.priority == key.priority, "priority");
            ICCL_ASSERT(sched->coll_attr.synchronous == key.synchronous, "synchronous");
            ICCL_ASSERT(sched->coll_param.ctype == key.ctype, "ctype");
            ICCL_ASSERT(sched->coll_param.dtype->type == key.dtype, "dtype");
            ICCL_ASSERT(sched->coll_param.comm == key.comm, "comm");

            if (sched->coll_param.ctype == iccl_coll_allgatherv)
                ICCL_ASSERT(sched->coll_param.send_count == key.count1, "count");
            else
                ICCL_ASSERT(sched->coll_param.count == key.count1, "count");

            if (sched->coll_param.ctype == iccl_coll_bcast || sched->coll_param.ctype == iccl_coll_reduce)
            {
                ICCL_ASSERT(sched->coll_param.root == key.root, "root");
            }

            if (sched->coll_param.ctype == iccl_coll_allreduce ||
                sched->coll_param.ctype == iccl_coll_reduce ||
                sched->coll_param.ctype == iccl_coll_sparse_allreduce)
            {
                ICCL_ASSERT(sched->coll_param.reduction == key.reduction, "reduction");
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
