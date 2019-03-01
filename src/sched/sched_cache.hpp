#pragma once

#include "sched/sched.hpp"
#include "common/utils/uthash.h"

struct mlsl_sched_cache_key
{
    mlsl_coll_type ctype;
    void *buf1;
    void *buf2;
    void *buf3;
    void *buf4; /* used in sparse collective to store recv value buf */
    size_t count1;
    size_t count2;
    size_t *count3; /* used in sparse collective to store recv index count */ 
    size_t *count4; /* used in sparse collective to store recv value count */ 
    mlsl_datatype_t dtype;
    mlsl_datatype_t itype; /* used in sparse collective to store index type */
    size_t root;
    mlsl_reduction_t reduction;
    const mlsl_comm* comm;
    mlsl_prologue_fn_t prologue_fn;
    mlsl_epilogue_fn_t epilogue_fn;
    mlsl_reduction_fn_t reduction_fn;
    size_t priority;
    int synchronous;
    char match_id[MLSL_MATCH_ID_MAX_LEN];
    /* TODO: extend to support more collectives */
};

struct mlsl_sched_cache_entry
{
    mlsl_sched *sched;
    mlsl_sched_cache_key key;
    UT_hash_handle hh;
};

struct mlsl_sched_cache
{
    mlsl_sched_cache_entry *head;
    mlsl_fastlock_t lock;
};

mlsl_status_t mlsl_sched_cache_create(mlsl_sched_cache **cache);
mlsl_status_t mlsl_sched_cache_free(mlsl_sched_cache *cache);
mlsl_status_t mlsl_sched_cache_get_entry(mlsl_sched_cache *cache, mlsl_sched_cache_key *key, mlsl_sched_cache_entry **entry);
mlsl_status_t mlsl_sched_cache_free_all(mlsl_sched_cache *cache);
