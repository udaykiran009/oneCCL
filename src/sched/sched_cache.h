#ifndef SCHED_CACHE_H
#define SCHED_CACHE_H

#include "sched/sched.h"
#include "common/utils/uthash.h"

struct mlsl_sched_cache_key
{
    mlsl_coll_type ctype;
    void *buf1;
    void *buf2;
    size_t count1;
    size_t count2;
    mlsl_data_type_t dtype;
    size_t root;
    mlsl_reduction_t reduction;
    char match_id[MLSL_MATCH_ID_MAX_LEN];
    /* TODO: extend to support more collectives */
};

typedef struct mlsl_sched_cache_key mlsl_sched_cache_key;

struct mlsl_sched_cache_entry
{
    mlsl_sched *sched;
    mlsl_sched_cache_key key;
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
mlsl_status_t mlsl_sched_cache_get_entry(mlsl_sched_cache *cache, mlsl_sched_cache_key *key, mlsl_sched_cache_entry **entry);
mlsl_status_t mlsl_sched_cache_free_all(mlsl_sched_cache *cache);

#endif /* SCHED_CACHE_H */
