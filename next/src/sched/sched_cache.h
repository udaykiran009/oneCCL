#ifndef SCHED_CACHE_H
#define SCHED_CACHE_H

#include "sched.h"
#include "uthash.h"

struct mlsl_sched_cache_entry
{
    mlsl_sched *origin_sched;
    mlsl_sched **worker_scheds;
    size_t worker_sched_count;
    UT_hash_handle hh;
};

typedef struct mlsl_sched_cache_entry mlsl_sched_cache_entry;

struct mlsl_sched_cache
{
    mlsl_sched_cache_entry *head;
};

typedef struct mlsl_sched_cache mlsl_sched_cache;

extern mlsl_sched_cache *global_sched_cache;

mlsl_status_t mlsl_sched_cache_create(mlsl_sched_cache **cache);
mlsl_status_t mlsl_sched_cache_free(mlsl_sched_cache *cache);
mlsl_status_t mlsl_sched_cache_get_entry(mlsl_sched_cache *cache, mlsl_sched *sched, mlsl_sched_cache_entry **entry);
mlsl_status_t mlsl_sched_cache_free_entry(mlsl_sched_cache *cache, mlsl_sched *sched);

#endif /* SCHED_CACHE_H */
