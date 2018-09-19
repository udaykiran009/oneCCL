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
    mlsl_sched_cache_free_all(cache);
    MLSL_FREE(cache);
    return mlsl_status_success;
}

mlsl_status_t mlsl_sched_cache_get_entry(mlsl_sched_cache *cache, mlsl_sched_cache_key *key, mlsl_sched_cache_entry **entry)
{
    mlsl_sched_cache_entry *e = NULL;
    HASH_FIND(hh, cache->head, key, sizeof(mlsl_sched_cache_key), e);
    if (!e)
    {
        e = MLSL_CALLOC(sizeof(mlsl_sched_cache_entry), "sched_cache_entry");
        memcpy(&(e->key), key, sizeof(mlsl_sched_cache_key));
        e->sched = NULL;
        HASH_ADD(hh, cache->head, key, sizeof(mlsl_sched_cache_key), e);
        MLSL_LOG(DEBUG, "didn't find sched in cache, add entry with NULL sched into cache");
    }
    else
        MLSL_LOG(DEBUG, "found sched in cache, %p", e->sched);
    *entry = e;

    return mlsl_status_success;
}

mlsl_status_t mlsl_sched_cache_free_all(mlsl_sched_cache *cache)
{
    mlsl_sched_cache_entry *current_entry, *tmp_entry;

    HASH_ITER(hh, cache->head, current_entry, tmp_entry)
    {
        HASH_DEL(cache->head, current_entry);
        mlsl_sched_free(current_entry->sched);
    }

    return mlsl_status_success;
}
