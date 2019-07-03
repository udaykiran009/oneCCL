#pragma once

#include "sched/entry/entry.hpp"

class epilogue_entry : public sched_entry
{
public:
    epilogue_entry() = delete;
    epilogue_entry(iccl_sched* sched,
                   iccl_epilogue_fn_t fn,
                   const void* in_buf,
                   size_t in_cnt,
                   iccl_datatype_internal_t in_dtype,
                   void* out_buf,
                   size_t expected_out_cnt,
                   iccl_datatype_internal_t out_dtype) :
        sched_entry(sched), fn(fn), in_buf(in_buf),
        in_cnt(in_cnt), in_dtype(in_dtype),
        out_buf(out_buf), expected_out_cnt(expected_out_cnt),
        out_dtype(out_dtype)
    {
        LOG_DEBUG("creating ", name(), " entry");
        pfields.add_available(iccl_sched_entry_field_in_buf);
        pfields.add_available(iccl_sched_entry_field_in_cnt);
        pfields.add_available(iccl_sched_entry_field_in_dtype);
    }

    void start_derived()
    {
        fn(in_buf, in_cnt, in_dtype->type, out_buf, &out_cnt, out_dtype->type);
        ICCL_ASSERT(expected_out_cnt == out_cnt, "incorrect values ", expected_out_cnt, " ", out_cnt);
        status = iccl_sched_entry_status_complete;
    }

    void* get_field_ptr(iccl_sched_entry_field_id id)
    {
        switch (id)
        {
            case iccl_sched_entry_field_in_buf: return &in_buf;
            case iccl_sched_entry_field_in_cnt: return &in_cnt;
            case iccl_sched_entry_field_in_dtype: return &in_dtype;
            default: ICCL_FATAL("unexpected id ", id);
        }
        return nullptr;
    }

    const char* name() const
    {
        return "EPILOGUE";
    }

protected:
    void dump_detail(std::stringstream& str) const
    {
        iccl_logger::format(str,
                            "in_dt ", iccl_datatype_get_name(in_dtype),
                            ", in_cnt ", in_cnt,
                            ", in_buf ", in_buf,
                            ", out_dt ", iccl_datatype_get_name(out_dtype),
                            ", out_cnt ", out_cnt,
                            ", out_buf ", out_buf,
                            ", fn ", fn,
                            ", exp_out_count ", expected_out_cnt,
                            "\n");
    }

private:
    iccl_epilogue_fn_t fn;
    const void* in_buf;
    size_t in_cnt;
    iccl_datatype_internal_t in_dtype;
    void* out_buf;
    size_t out_cnt;
    size_t expected_out_cnt;
    iccl_datatype_internal_t out_dtype;
};
