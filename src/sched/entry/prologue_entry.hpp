#pragma once

#include "sched/entry/entry.hpp"

class prologue_entry : public sched_entry
{
public:
    static constexpr const char *entry_class_name() noexcept
    {
        return "PROLOGUE";
    }

    prologue_entry() = delete;
    prologue_entry(ccl_sched* sched,
                   ccl_prologue_fn_t fn,
                   const ccl_buffer in_buf,
                   size_t in_cnt,
                   ccl_datatype_internal_t in_dtype,
                   void** out_buf,
                   size_t* out_cnt,
                   ccl_datatype_t* out_dtype,
                   size_t* out_dtype_size) :
        sched_entry(sched), fn(fn), in_buf(in_buf),
        in_cnt(in_cnt), in_dtype(in_dtype),
        out_buf(out_buf), out_cnt(out_cnt),
        out_dtype(out_dtype), out_dtype_size(out_dtype_size)
    {
    }

    void start_derived() override
    {
        size_t in_bytes = in_cnt * ccl_datatype_get_size(in_dtype);
        fn(in_buf.get_ptr(in_bytes), in_cnt, in_dtype->type, out_buf, out_cnt, out_dtype, out_dtype_size);
        status = ccl_sched_entry_status_complete;
    }

    const char* name() const override
    {
        return entry_class_name();
    }

protected:
    void dump_detail(std::stringstream& str) const override
    {
        ccl_logger::format(str,
                           "in_dt ", ccl_datatype_get_name(in_dtype),
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
    ccl_prologue_fn_t fn;
    ccl_buffer in_buf;
    size_t in_cnt;
    ccl_datatype_internal_t in_dtype;
    void** out_buf;
    size_t* out_cnt;
    ccl_datatype_t* out_dtype;
    size_t* out_dtype_size;
};
