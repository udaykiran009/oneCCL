#pragma once

#include "sched/entry_types/entry.hpp"

class copy_entry : public sched_entry
{
public:
    copy_entry() = delete;
    copy_entry(mlsl_sched* sched,
               const void* in_buf,
               void* out_buf,
               size_t cnt,
               mlsl_datatype_internal_t dtype) :
        sched_entry(sched), in_buf(in_buf), out_buf(out_buf), cnt(cnt), dtype(dtype)
    {
        pfields.add_available(mlsl_sched_entry_field_in_buf);
        pfields.add_available(mlsl_sched_entry_field_cnt);
        pfields.add_available(mlsl_sched_entry_field_dtype);
    }

    void start_derived()
    {
        auto comp_status = mlsl_comp_copy(in_buf, out_buf, cnt, dtype);
        MLSL_ASSERT(comp_status == mlsl_status_success, "bad status %d", comp_status);
        status = mlsl_sched_entry_status_complete;
    }

    void* get_field_ptr(mlsl_sched_entry_field_id id)
    {
        switch (id)
        {
            case mlsl_sched_entry_field_in_buf: return &in_buf;
            case mlsl_sched_entry_field_cnt: return &cnt;
            case mlsl_sched_entry_field_dtype: return &dtype;
            default: MLSL_FATAL("unexpected id %d", id);
        }
        return nullptr;
    }

    const char* name() const
    {
        return "COPY";
    }

protected:
    char* dump_detail(char* dump_buf) const
    {
        auto bytes_written = sprintf(dump_buf, "dt %s, cnt %zu, in_buf %p, out_buf %p\n",
                                     mlsl_datatype_get_name(dtype), cnt, in_buf, out_buf);
        return dump_buf + bytes_written;
    }

private:
    const void* in_buf;
    void* out_buf;
    size_t cnt;
    mlsl_datatype_internal_t dtype;
};
