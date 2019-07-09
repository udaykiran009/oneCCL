#pragma once

#include "sched/entry/entry.hpp"

class copy_entry : public sched_entry
{
public:
    copy_entry() = delete;
    copy_entry(iccl_sched* sched,
               iccl_buf_placeholder in_buf,
               iccl_buf_placeholder out_buf,
               size_t cnt,
               iccl_datatype_internal_t dtype) :
        sched_entry(sched), in_buf(in_buf), out_buf(out_buf), cnt(cnt), dtype(dtype)
    {
        LOG_DEBUG("creating ", name(), " entry");
        pfields.add_available(iccl_sched_entry_field_in_buf);
        pfields.add_available(iccl_sched_entry_field_cnt);
        pfields.add_available(iccl_sched_entry_field_dtype);
    }

    void start_derived()
    {
        auto comp_status = iccl_comp_copy(in_buf.get_ptr(), out_buf.get_ptr(), cnt, dtype);
        ICCL_ASSERT(comp_status == iccl_status_success, "bad status ", comp_status);
        status = iccl_sched_entry_status_complete;
    }

    void* get_field_ptr(iccl_sched_entry_field_id id)
    {
        switch (id)
        {
            case iccl_sched_entry_field_in_buf: return &in_buf;
            case iccl_sched_entry_field_cnt: return &cnt;
            case iccl_sched_entry_field_dtype: return &dtype;
            default: ICCL_FATAL("unexpected id ", id);
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
        iccl_logger::format(str,
                            "dt ", iccl_datatype_get_name(dtype),
                            ", cnt ", cnt,
                            ", in_buf ", in_buf,
                            ", out_buf ", out_buf,
                            "\n");
    }

private:
    iccl_buf_placeholder in_buf;
    iccl_buf_placeholder out_buf;
    size_t cnt;
    iccl_datatype_internal_t dtype;
};
