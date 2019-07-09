#pragma once

#include "common/utils/spinlock.hpp"
#include "comp/comp.hpp"
#include "sched/sched.hpp"

#include <cstring>
#include <unordered_map>

#define ICCL_SCHED_CACHE_INITIAL_BUCKET_COUNT (4096)

class iccl_sched_key
{
public:
    iccl_sched_key() = default;
    ~iccl_sched_key() = default;
    iccl_sched_key(const iccl_sched_key& other) = default;

    iccl_sched_key& operator= (const iccl_sched_key& other) = delete;
    void* buf1 = nullptr;
    void* buf2 = nullptr;
    void* buf3 = nullptr;
    void* buf4 = nullptr; /* used in sparse collective to store recv value buf */

    iccl_coll_type ctype = iccl_coll_none;
    iccl_datatype_t dtype = iccl_dtype_char;
    iccl_datatype_t itype = iccl_dtype_char; /* used in sparse collective to store index type */
    iccl_reduction_t reduction = iccl_reduction_sum;
    size_t count1 = 0;
    size_t count2 = 0;
    size_t* count3 = nullptr; /* used in sparse collective to store recv index count */
    size_t* count4 = nullptr; /* used in sparse collective to store recv value count */
    size_t root = 0;
    const iccl_comm* comm = nullptr;
    iccl_prologue_fn_t prologue_fn = nullptr;
    iccl_epilogue_fn_t epilogue_fn = nullptr;
    iccl_reduction_fn_t reduction_fn = nullptr;
    size_t priority = 0;
    int synchronous = 0;
    int filler = 0; /* to zeroize 4 bytes hole */
    std::string match_id{}; /* should always be last field */

    bool operator== (const iccl_sched_key& k) const
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
        LOG_DEBUG( "ctype ", iccl_coll_type_to_str(ctype),
                   ", dtype ", iccl_datatype_get(dtype)->name,
                   ", itype ", iccl_datatype_get(itype)->name,
                   ", reduction ", iccl_reduction_to_str(reduction),
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

class iccl_sched_key_hasher
{
public:
    size_t operator()(const iccl_sched_key& k) const
    {
        size_t hash_value = k.ctype + k.dtype + k.itype + k.reduction + k.count1 +
               k.count2 + k.root + k.priority + k.synchronous +
//               (size_t)k.buf1 + (size_t)k.buf2 + (size_t)k.buf3 + (size_t)k.buf4 +
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

class iccl_sched_cache
{
public:
    iccl_sched_cache() = default;
    ~iccl_sched_cache()
    {
        remove_all();
    }

    iccl_sched_cache(const iccl_sched_cache& other) = delete;
    iccl_sched_cache& operator= (const iccl_sched_cache& other) = delete;

    iccl_sched* find(iccl_sched_key& key);
    void add(iccl_sched_key& key, iccl_sched* sched);

private:
    void remove_all();

    using sched_cache_lock_t = iccl_spinlock;
    sched_cache_lock_t guard{};
    using sched_table_t = std::unordered_map<iccl_sched_key, iccl_sched*, iccl_sched_key_hasher>;
    sched_table_t table { ICCL_SCHED_CACHE_INITIAL_BUCKET_COUNT };
};
