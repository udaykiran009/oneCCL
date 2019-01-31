#pragma once

#include "sched/entry_types/entry.hpp"

class prologue_entry : public sched_entry
{
public:
    prologue_entry() = delete;
    prologue_entry(mlsl_sched* sched,
                   mlsl_prologue_fn_t fn,
                   const void* in_buf,
                   size_t in_cnt,
                   mlsl_datatype_internal_t in_dtype,
                   void** out_buf,
                   size_t* out_cnt,
                   mlsl_datatype_t* out_dtype,
                   size_t* out_dtype_size) :
        sched_entry(sched), fn(fn), in_buf(in_buf),
        in_cnt(in_cnt), in_dtype(in_dtype),
        out_buf(out_buf), out_cnt(out_cnt),
        out_dtype(out_dtype), out_dtype_size(out_dtype_size)
    {}

    void start_derived()
    {
        fn(in_buf, in_cnt, in_dtype->type, out_buf, out_cnt, out_dtype, out_dtype_size);
        status = mlsl_sched_entry_status_complete;
    }

    const char* name() const
    {
        return "PROLOGUE";
    }

protected:
    char* dump_detail(char* dump_buf) const
    {
        auto bytes_written = sprintf(dump_buf,
                                     "in_dt %s, in_cnt %zu, in_buf %p, out_dt %p, out_dtype_size %p, "
                                     "out_cnt %p, out_buf %p, fn %p\n",
                                     mlsl_datatype_get_name(in_dtype), in_cnt, in_buf,
                                     out_dtype, out_dtype_size, out_cnt, out_buf, fn);
        return dump_buf + bytes_written;
    }

private:
    mlsl_prologue_fn_t fn;
    const void* in_buf;
    size_t in_cnt;
    mlsl_datatype_internal_t in_dtype;
    void** out_buf;
    size_t* out_cnt;
    mlsl_datatype_t* out_dtype;
    size_t* out_dtype_size;
};
