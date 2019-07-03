#pragma once

#include "iccl_types.h"

#include <map>
#include <set>

typedef iccl_status_t(*iccl_sched_entry_field_function_t) (const void*, void*);

class sched_entry;

enum iccl_sched_entry_field_id
{
    iccl_sched_entry_field_buf,
    iccl_sched_entry_field_send_buf,
    iccl_sched_entry_field_recv_buf,
    iccl_sched_entry_field_cnt,
    iccl_sched_entry_field_dtype,
    iccl_sched_entry_field_src_mr,
    iccl_sched_entry_field_dst_mr,
    iccl_sched_entry_field_in_buf,
    iccl_sched_entry_field_in_cnt,
    iccl_sched_entry_field_in_dtype
};

class postponed_field
{
public:
    postponed_field(iccl_sched_entry_field_id id,
                    iccl_sched_entry_field_function_t fn,
                    const void* ctx,
                    bool update_once) :
        id(id), fn(fn), ctx(ctx), update_once(update_once)
    {}

    iccl_sched_entry_field_id id;
    iccl_sched_entry_field_function_t fn;
    const void* ctx;
    bool update_once;
};

class postponed_fields
{
public:
    postponed_fields(sched_entry* entry) : entry(entry)
    {}

    void add_available(iccl_sched_entry_field_id id);
    void add(iccl_sched_entry_field_id id,
             iccl_sched_entry_field_function_t fn,
             const void* ctx,
             bool update_once);
    void update();

private:
    sched_entry* entry;
    std::set<iccl_sched_entry_field_id> available_fields{};
    std::map<iccl_sched_entry_field_id, postponed_field> fields{};
};
