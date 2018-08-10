#include "sched_cache.h"

mlsl_sched_cache *global_sched_cache = NULL;

mlsl_status_t mlsl_sched_cache_create(mlsl_sched_cache **cache)
{
    mlsl_sched_cache *c = MLSL_CALLOC(sizeof(mlsl_sched_cache), "sched_cache");
    *cache = c;
    return mlsl_status_success;
}

mlsl_status_t mlsl_sched_cache_free(mlsl_sched_cache *cache)
{
    MLSL_FREE(cache);
    return mlsl_status_success;
}

mlsl_status_t mlsl_sched_cache_get_entry(mlsl_sched_cache *cache, mlsl_sched *sched, mlsl_sched_cache_entry **entry)
{
    mlsl_sched_cache_entry *e;
    HASH_FIND_PTR(cache->head, &sched, e);
    if (!e)
    {
        e = MLSL_CALLOC(sizeof(mlsl_sched_cache_entry), "sched_cache_entry");
        e->origin_sched = sched;
        HASH_ADD_PTR(cache->head, origin_sched, e);
        MLSL_LOG(DEBUG, "not found sched in cache, %p", sched);
    }
    else
        MLSL_LOG(DEBUG, "found sched in cache, %p", sched);
    *entry = e;

    return mlsl_status_success;
}

mlsl_status_t mlsl_sched_cache_free_entry(mlsl_sched_cache *cache, mlsl_sched *sched)
{
    mlsl_sched_cache_entry *e;
    HASH_FIND_PTR(cache->head, &sched, e);
    if (e)
    {
        HASH_DEL(cache->head, e);
        size_t idx;
        for (idx = 0; idx < e->worker_sched_count; idx++)
        {
            mlsl_sched_free(e->worker_scheds[idx]);
        }
        MLSL_FREE(e->worker_scheds);
        MLSL_FREE(e);
    }

    return mlsl_status_success;
}
