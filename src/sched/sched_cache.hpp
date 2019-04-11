#pragma once

#include "common/utils/spinlock.hpp"
#include "comp/comp.hpp"
#include "sched/sched.hpp"

#include <cstring>
#include <unordered_map>

#define MLSL_SCHED_CACHE_INITIAL_BUCKET_COUNT (4096)

class mlsl_sched_key
{
public:
    mlsl_sched_key() = default;
    ~mlsl_sched_key() = default;
    mlsl_sched_key(const mlsl_sched_key& other) = default;

    mlsl_sched_key& operator= (const mlsl_sched_key& other) = delete;

    mlsl_coll_type ctype = mlsl_coll_none;
    mlsl_datatype_t dtype = mlsl_dtype_char;
    mlsl_datatype_t itype = mlsl_dtype_char; /* used in sparse collective to store index type */
    mlsl_reduction_t reduction = mlsl_reduction_sum;
    void* buf1 = nullptr;
    void* buf2 = nullptr;
    void* buf3 = nullptr;
    void* buf4 = nullptr; /* used in sparse collective to store recv value buf */
    size_t count1 = 0;
    size_t count2 = 0;
    size_t* count3 = nullptr; /* used in sparse collective to store recv index count */
    size_t* count4 = nullptr; /* used in sparse collective to store recv value count */
    size_t root = 0;
    const mlsl_comm* comm = nullptr;
    mlsl_prologue_fn_t prologue_fn = nullptr;
    mlsl_epilogue_fn_t epilogue_fn = nullptr;
    mlsl_reduction_fn_t reduction_fn = nullptr;
    size_t priority = 0;
    int synchronous = 0;
    int filler = 0; /* to zeroize 4 bytes hole */
    std::string match_id{}; /* should always be last field */

    bool operator== (const mlsl_sched_key& k) const
    { 
        char* first_field1 = (char*)&ctype;
        char* last_field1 = (char*)&match_id;
        void* first_field2 = (char*)&k.ctype;
        size_t bytes_to_compare = last_field1 - first_field1;
        bool is_equal = !memcmp(first_field1, first_field2, bytes_to_compare) &&
                        !match_id.compare(k.match_id);
        LOG_DEBUG("is_equal ", is_equal, ", bytes_to_compare ", bytes_to_compare);
        print();
        k.print();
        return is_equal;
    } 

    void print() const
    {
        LOG_DEBUG( "ctype ", mlsl_coll_type_to_str(ctype),
                   ", dtype ", mlsl_datatype_get(dtype)->name,
                   ", itype ", mlsl_datatype_get(itype)->name,
                   ", reduction ", mlsl_reduction_to_str(reduction),
                   ", buf1 ", buf1,
                   ", buf2 ", buf2,
                   ", buf3 ", buf3,
                   ", buf4 ", buf4,
                   ", count1 ", count1,
                   ", count2 ", count2,
                   ", count3 ", count3,
                   ", count4 ", count4,
                   ", root ", root,
                   ", comm ", comm,
                   ", prologue_fn ", prologue_fn,
                   ", epilogue_fn ", epilogue_fn,
                   ", reduction_fn ", reduction_fn,
                   ", priority ", priority,
                   ", sync ", synchronous,
                   ", match_id ", match_id);
    }
};

class mlsl_sched_key_hasher
{
public:
    size_t operator()(const mlsl_sched_key& k) const
    {
        size_t hash_value = k.ctype + k.dtype + k.itype + k.reduction + k.count1 +
               k.count2 + k.root + k.priority + k.synchronous +
               (size_t)k.buf1 + (size_t)k.buf2 + (size_t)k.buf3 + (size_t)k.buf4 +
               (size_t)k.count3 + (size_t)k.count4 + (size_t)k.comm +
               (size_t)k.prologue_fn + (size_t)k.epilogue_fn + (size_t)k.reduction_fn +
               string_hasher(k.match_id);
        LOG_DEBUG("hash_value ", hash_value);
        k.print();
        return hash_value;
    }
private:
    std::hash<std::string> string_hasher{};
};

class mlsl_sched_cache
{
public:
    mlsl_sched_cache() = default;
    ~mlsl_sched_cache()
    {
        remove_all();
    }

    mlsl_sched_cache(const mlsl_sched_cache& other) = delete;
    mlsl_sched_cache& operator= (const mlsl_sched_cache& other) = delete;

    mlsl_sched* find(mlsl_sched_key& key);
    void add(mlsl_sched_key& key, mlsl_sched* sched);

private:
    void remove_all();

    using sched_cache_lock_t = mlsl_spinlock;
    sched_cache_lock_t guard{};
    using sched_table_t = std::unordered_map<mlsl_sched_key, mlsl_sched*, mlsl_sched_key_hasher>;
    sched_table_t table { MLSL_SCHED_CACHE_INITIAL_BUCKET_COUNT };
};
