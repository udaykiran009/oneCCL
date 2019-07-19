#pragma once

#include "sched/entry/entry.hpp"

class prologue_entry : public sched_entry
{
public:
    prologue_entry() = delete;
    prologue_entry(iccl_sched* sched,
                   iccl_prologue_fn_t fn,
                   const iccl_buffer in_buf,
                   size_t in_cnt,
                   iccl_datatype_internal_t in_dtype,
                   void** out_buf,
                   size_t* out_cnt,
                   iccl_datatype_t* out_dtype,
                   size_t* out_dtype_size) :
        sched_entry(sched), fn(fn), in_buf(in_buf),
        in_cnt(in_cnt), in_dtype(in_dtype),
        out_buf(out_buf), out_cnt(out_cnt),
        out_dtype(out_dtype), out_dtype_size(out_dtype_size)
    {
        LOG_DEBUG("creating ", name(), " entry");
    }

    void start_derived()
    {
        size_t in_bytes = in_cnt * iccl_datatype_get_size(in_dtype);
        fn(in_buf.get_ptr(in_bytes), in_cnt, in_dtype->type, out_buf, out_cnt, out_dtype, out_dtype_size);
        status = iccl_sched_entry_status_complete;
    }

    const char* name() const
    {
        return "PROLOGUE";
    }

protected:
    void dump_detail(std::stringstream& str) const
    {
        iccl_logger::format(str,
                            "in_dt ", iccl_datatype_get_name(in_dtype),
                            ", in_cnt ", in_cnt,
                            ", in_buf ", in_buf,
                            ", out_dt ", out_dtype,
                            ", out_dtype_size ", out_dtype_size,
                            ", out_cnt ", out_cnt,
                            ", out_buf ", out_buf,
                            ", fn ", fn,
                            "\n");
    }

private:
    iccl_prologue_fn_t fn;
    iccl_buffer in_buf;
    size_t in_cnt;
    iccl_datatype_internal_t in_dtype;
    void** out_buf;
    size_t* out_cnt;
    iccl_datatype_t* out_dtype;
    size_t* out_dtype_size;
};
