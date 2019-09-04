#include "common/env/env.hpp"
#include "sched/sched_cache.hpp"

const char* ccl_cache_key_to_str(ccl_cache_key key)
{
    switch (key)
    {
        case ccl_cache_key_full:
            return "full";
        case ccl_cache_key_match_id:
            return "match_id";
        default:
            CCL_FATAL("unknown cache_key ", key);
    }
}

bool ccl_sched_key::operator== (const ccl_sched_key& k) const
{
    char* first_field1 = (char*)&ctype;
    char* last_field1 = (char*)&match_id;
    void* first_field2 = (char*)&k.ctype;
    size_t bytes_to_compare = last_field1 - first_field1;
    bool is_fields_equal = (env_data.cache_key == ccl_cache_key_full) ?
        !memcmp(first_field1, first_field2, bytes_to_compare) : 1;

    bool is_equal = is_fields_equal && !match_id.compare(k.match_id);
    LOG_DEBUG("is_equal ", is_equal);
    print();
    k.print();
    return is_equal;
}

size_t ccl_sched_key_hasher::operator()(const ccl_sched_key& k) const
{
    if (k.has_hasher_result)
        return k.get_hasher_result();

    size_t hash_value = string_hasher(k.match_id);
    if (env_data.cache_key == ccl_cache_key_full)
    {
        hash_value += k.ctype + k.dtype + k.itype + k.reduction +
            k.count1 + k.count2 + k.root + (size_t)k.buf +
            (size_t)k.count3 + (size_t)k.count4 + (size_t)k.comm +
            (size_t)k.prologue_fn + (size_t)k.epilogue_fn + (size_t)k.reduction_fn;
    }

    const_cast<ccl_sched_key&>(k).set_hasher_result(hash_value);

    LOG_DEBUG("hash_value ", hash_value);
    k.print();

    return hash_value;
}

ccl_master_sched* ccl_sched_cache::find(ccl_sched_key& key)
{
    ccl_master_sched* sched = nullptr;
    {
        std::lock_guard<sched_cache_lock_t> lock{guard};
        sched_table_t::iterator it = table.find(key);
        if (it != table.end())
        {
            sched = it->second;
        }
    }

    if (sched)
    {
        LOG_DEBUG("found sched in cache, ",sched);
        if (env_data.cache_key != ccl_cache_key_full)
        {
            LOG_DEBUG("do check for found sched");
            CCL_ASSERT(sched->coll_attr.prologue_fn == key.prologue_fn, "prologue_fn");
            CCL_ASSERT(sched->coll_attr.epilogue_fn == key.epilogue_fn, "epilogue_fn");
            CCL_ASSERT(sched->coll_attr.reduction_fn == key.reduction_fn, "reduction_fn");
            CCL_ASSERT(sched->coll_param.ctype == key.ctype, "ctype");
            CCL_ASSERT(sched->coll_param.dtype->type == key.dtype, "dtype");
            CCL_ASSERT(sched->coll_param.comm == key.comm, "comm");

            if (sched->coll_param.ctype == ccl_coll_allgatherv)
                CCL_ASSERT(sched->coll_param.send_count == key.count1, "count");
            else
                CCL_ASSERT(sched->coll_param.count == key.count1, "count");

            if (sched->coll_param.ctype == ccl_coll_bcast || sched->coll_param.ctype == ccl_coll_reduce)
            {
                CCL_ASSERT(sched->coll_param.root == key.root, "root");
            }

            if (sched->coll_param.ctype == ccl_coll_allreduce ||
                sched->coll_param.ctype == ccl_coll_reduce ||
                sched->coll_param.ctype == ccl_coll_sparse_allreduce)
            {
                CCL_ASSERT(sched->coll_param.reduction == key.reduction, "reduction");
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

void ccl_sched_cache::add(ccl_sched_key&& key, ccl_master_sched* sched)
{
    {
        std::lock_guard<sched_cache_lock_t> lock{guard};
        auto emplace_result = table.emplace(std::move(key), sched);
        CCL_ASSERT(emplace_result.second);
    }

    LOG_DEBUG("size ", table.size(),
              ", bucket_count ", table.bucket_count(),
              ", load_factor ", table.load_factor(),
              ", max_load_factor ", table.max_load_factor());
}

void ccl_sched_cache::remove_all()
{
    std::lock_guard<sched_cache_lock_t> lock{guard};
    for (auto it = table.begin(); it != table.end(); ++it)
    {
        ccl_master_sched* sched = it->second;
        CCL_ASSERT(sched);
        LOG_DEBUG("remove sched ", sched, " from cache");
        delete sched;
    }
    table.clear();
}
