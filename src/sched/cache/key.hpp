#pragma once

#include "coll/coll.hpp"
#include "comp/comp.hpp"

#include <map>
#include <unordered_map>

enum ccl_cache_key_type
{
    ccl_cache_key_full,
    ccl_cache_key_match_id,

    ccl_cache_key_last_value
};

const char* ccl_cache_key_type_to_str(ccl_cache_key_type type);

class ccl_sched_key
{
private:
    size_t hasher_result = 0;

public:
    ccl_sched_key() = default;
    ~ccl_sched_key() = default;
    ccl_sched_key(ccl_sched_key&& other) = default;
    ccl_sched_key& operator= (ccl_sched_key&& other) = default;

    ccl_sched_key(const ccl_coll_param& param, const ccl_coll_attr& attr);
    void set(const ccl_coll_param& param, const ccl_coll_attr& attr);
    bool check(const ccl_coll_param& param, const ccl_coll_attr& attr) const;

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

    struct ccl_sched_key_inner_fields
    {
        ccl_coll_type ctype = ccl_coll_internal;
        void* buf1 = nullptr; /* non-data buffer which can be used for caching */
        void* buf2 = nullptr; /* non-data buffer which can be used for caching */
        ccl_datatype_t dtype = ccl_dtype_char;
        ccl_datatype_t itype = ccl_dtype_char; /* used in sparse collective to store index type */
        ccl::reduction reduction = ccl::reduction::sum;
        size_t count1 = 0;
        size_t count2 = 0;
        size_t count3 = 0; /* used in sparse collective to store recv index count */
        size_t count4 = 0; /* used in sparse collective to store recv value count */
        size_t root = 0;
        const ccl_comm* comm = nullptr;
        ccl::prologue_fn_t prologue_fn = nullptr;
        ccl::epilogue_fn_t epilogue_fn = nullptr;
        ccl::reduction_fn_t reduction_fn = nullptr;
    };

    /* inner structure for bit comparison */
    ccl_sched_key_inner_fields f;

    std::string match_id{};

    bool operator== (const ccl_sched_key& k) const;

    void print() const;

    static std::map<ccl_cache_key_type, std::string> key_type_names;
};

class ccl_sched_key_hasher
{
public:
    size_t operator()(const ccl_sched_key& k) const;

private:
    std::hash<std::string> string_hasher{};
};
