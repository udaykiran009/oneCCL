#pragma once

#include "ccl_types.h"

#include <map>
#include <set>

typedef ccl_status_t(*ccl_sched_entry_field_function_t) (const void*, void*);

class sched_entry;

enum ccl_sched_entry_field_id
{
    ccl_sched_entry_field_buf,
    ccl_sched_entry_field_send_buf,
    ccl_sched_entry_field_recv_buf,
    ccl_sched_entry_field_cnt,
    ccl_sched_entry_field_dtype,
    ccl_sched_entry_field_src_mr,
    ccl_sched_entry_field_dst_mr,
    ccl_sched_entry_field_in_buf,
    ccl_sched_entry_field_in_cnt,
    ccl_sched_entry_field_in_dtype
};

class postponed_field
{
public:
    postponed_field(ccl_sched_entry_field_id id,
                    ccl_sched_entry_field_function_t fn,
                    const void* ctx,
                    bool update_once) :
        id(id), fn(fn), ctx(ctx), update_once(update_once)
    {}

    ccl_sched_entry_field_id id;
    ccl_sched_entry_field_function_t fn;
    const void* ctx;
    bool update_once;
};

class postponed_fields
{
public:
    postponed_fields(sched_entry* entry) : entry(entry)
    {}

    void add_available(ccl_sched_entry_field_id id);
    void add(ccl_sched_entry_field_id id,
             ccl_sched_entry_field_function_t fn,
             const void* ctx,
             bool update_once);
    void update();

private:
    sched_entry* entry;
    std::set<ccl_sched_entry_field_id> available_fields{};
    std::map<ccl_sched_entry_field_id, postponed_field> fields{};
};
