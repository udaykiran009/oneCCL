#pragma once

#include "common/utils/spinlock.hpp"
#include "comp/comp.hpp"
#include "sched/sched.hpp"

#include <cstring>
#include <unordered_map>

#define CCL_SCHED_CACHE_INITIAL_BUCKET_COUNT (4096)

enum ccl_cache_key
{
    ccl_cache_key_full,
    ccl_cache_key_match_id,

    ccl_cache_key_last_value
};

const char* ccl_cache_key_to_str(ccl_cache_key key);

class ccl_sched_key
{
private:
    size_t hasher_result = 0;

public:
    ccl_sched_key() = default;
    ~ccl_sched_key() = default;
    ccl_sched_key(const ccl_sched_key& other) = default;

    ccl_sched_key& operator= (const ccl_sched_key& other) = delete;

    size_t get_hasher_result() const
    {
        return hasher_result;
    }

    void set_hasher_result(size_t value)
    {
        has_hasher_result = true;
        hasher_result = value;
    }

    bool has_hasher_result = false;

    void* buf = nullptr; /* non-data buffer which can be used for caching */
    ccl_coll_type ctype = ccl_coll_none;
    ccl_datatype_t dtype = ccl_dtype_char;
    ccl_datatype_t itype = ccl_dtype_char; /* used in sparse collective to store index type */
    ccl_reduction_t reduction = ccl_reduction_sum;
    size_t count1 = 0;
    size_t count2 = 0;
    size_t* count3 = nullptr; /* used in sparse collective to store recv index count */
    size_t* count4 = nullptr; /* used in sparse collective to store recv value count */
    size_t root = 0;
    const ccl_comm* comm = nullptr;
    ccl_prologue_fn_t prologue_fn = nullptr;
    ccl_epilogue_fn_t epilogue_fn = nullptr;
    ccl_reduction_fn_t reduction_fn = nullptr;

    std::string match_id{}; /* should always be last field */

    bool operator== (const ccl_sched_key& k) const;

    void print() const
    {
        LOG_DEBUG( "ctype ", ccl_coll_type_to_str(ctype),
                   ", dtype ", ccl_datatype_get(dtype)->name,
                   ", itype ", ccl_datatype_get(itype)->name,
                   ", reduction ", ccl_reduction_to_str(reduction),
                   ", buf ", buf,
                   ", count1 ", count1,
                   ", count2 ", count2,
                   ", count3 ", count3,
                   ", count4 ", count4,
                   ", root ", root,
                   ", comm ", comm,
                   ", prologue_fn ", (void*)prologue_fn,
                   ", epilogue_fn ", (void*)epilogue_fn,
                   ", reduction_fn ", (void*)reduction_fn,
                   ", match_id ", match_id);
    }
};

class ccl_sched_key_hasher
{
public:
    size_t operator()(const ccl_sched_key& k) const;

private:
    std::hash<std::string> string_hasher{};
};

class ccl_sched_cache
{
public:
    ccl_sched_cache() = default;
    ~ccl_sched_cache()
    {
        remove_all();
    }

    ccl_sched_cache(const ccl_sched_cache& other) = delete;
    ccl_sched_cache& operator= (const ccl_sched_cache& other) = delete;

    ccl_sched* find(ccl_sched_key& key);
    void add(ccl_sched_key& key, ccl_sched* sched);

private:
    void remove_all();

    using sched_cache_lock_t = ccl_spinlock;
    sched_cache_lock_t guard{};
    using sched_table_t = std::unordered_map<ccl_sched_key, ccl_sched*, ccl_sched_key_hasher>;
    sched_table_t table { CCL_SCHED_CACHE_INITIAL_BUCKET_COUNT };
};
