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
        LOG_DEBUG("creating ", name(), " entry");
        pfields.add_available(mlsl_sched_entry_field_in_buf);
        pfields.add_available(mlsl_sched_entry_field_cnt);
        pfields.add_available(mlsl_sched_entry_field_dtype);
    }

    void start_derived()
    {
        auto comp_status = mlsl_comp_copy(in_buf, out_buf, cnt, dtype);
        MLSL_ASSERT(comp_status == mlsl_status_success, "bad status ", comp_status);
        status = mlsl_sched_entry_status_complete;
    }

    void* get_field_ptr(mlsl_sched_entry_field_id id)
    {
        switch (id)
        {
            case mlsl_sched_entry_field_in_buf: return &in_buf;
            case mlsl_sched_entry_field_cnt: return &cnt;
            case mlsl_sched_entry_field_dtype: return &dtype;
            default: MLSL_FATAL("unexpected id ", id);
        }
        return nullptr;
    }

    const char* name() const
    {
        return "COPY";
    }

protected:
    void dump_detail(std::stringstream& str) const
    {
        mlsl_logger::format(str,
                            "dt ", mlsl_datatype_get_name(dtype),
                            ", cnt ", cnt,
                            ", in_buf ", in_buf,
                            ", out_buf ", out_buf,
                            "\n");
    }

private:
    const void* in_buf;
    void* out_buf;
    size_t cnt;
    mlsl_datatype_internal_t dtype;
};
