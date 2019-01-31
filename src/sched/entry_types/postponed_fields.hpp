#pragma once

#include "mlsl_types.h"

#include <map>
#include <set>

typedef mlsl_status_t(*mlsl_sched_entry_field_function_t) (const void*, void*);

class sched_entry;

enum mlsl_sched_entry_field_id
{
    mlsl_sched_entry_field_buf,
    mlsl_sched_entry_field_send_buf,
    mlsl_sched_entry_field_recv_buf,
    mlsl_sched_entry_field_cnt,
    mlsl_sched_entry_field_dtype,
    mlsl_sched_entry_field_src_mr,
    mlsl_sched_entry_field_dst_mr,
    mlsl_sched_entry_field_in_buf,
    mlsl_sched_entry_field_in_cnt,
    mlsl_sched_entry_field_in_dtype
};

class postponed_field
{
public:
    postponed_field(mlsl_sched_entry_field_id id,
                    mlsl_sched_entry_field_function_t fn,
                    const void* ctx,
                    bool update_once) :
        id(id), fn(fn), ctx(ctx), update_once(update_once)
    {}

    mlsl_sched_entry_field_id id;
    mlsl_sched_entry_field_function_t fn;
    const void* ctx;
    bool update_once;
};

class postponed_fields
{
public:
    postponed_fields(sched_entry* entry) : entry(entry)
    {}

    void add_available(mlsl_sched_entry_field_id id);
    void add(mlsl_sched_entry_field_id id,
             mlsl_sched_entry_field_function_t fn,
             const void* ctx,
             bool update_once);
    void update();

private:
    sched_entry* entry;
    std::set<mlsl_sched_entry_field_id> available_fields{};
    std::map<mlsl_sched_entry_field_id, postponed_field> fields{};
};
