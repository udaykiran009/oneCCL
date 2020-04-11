#pragma once

#include "sched/entry/entry.hpp"

class prologue_entry : public sched_entry
{
public:
    static constexpr const char* class_name() noexcept
    {
        return "PROLOGUE";
    }

    prologue_entry() = delete;
    prologue_entry(ccl_sched* sched,
                   ccl_prologue_fn_t fn,
                   const ccl_buffer in_buf,
                   size_t in_cnt,
                   const ccl_datatype& in_dtype,
                   void** out_buf,
                   size_t* out_cnt,
                   ccl_datatype_t* out_dtype_idx) :
        sched_entry(sched), fn(fn), in_buf(in_buf),
        in_cnt(in_cnt), in_dtype(in_dtype),
        out_buf(out_buf), out_cnt(out_cnt),
        out_dtype_idx(out_dtype_idx)
    {
    }

    void start() override
    {
        size_t in_bytes = in_cnt * in_dtype.size();
        size_t offset = in_buf.get_offset();
        const ccl_fn_context_t context = { sched->coll_attr.match_id.c_str(), offset };
        fn(in_buf.get_ptr(in_bytes), in_cnt, in_dtype.idx(), out_buf, out_cnt, out_dtype_idx, &context);
        status = ccl_sched_entry_status_complete;
    }

    const char* name() const override
    {
        return class_name();
    }

protected:
    void dump_detail(std::stringstream& str) const override
    {
        ccl_logger::format(str,
                           "in_dt ", global_data.dtypes->name(in_dtype),
                           ", in_cnt ", in_cnt,
                           ", in_buf ", in_buf,
                           ", out_dt ", global_data.dtypes->name(*out_dtype_idx),
                           ", out_dtype_size ", global_data.dtypes->get(*out_dtype_idx).size(),
                           ", out_cnt ", out_cnt,
                           ", out_buf ", out_buf,
                           ", fn ", fn,
                           "\n");
    }

private:
    ccl_prologue_fn_t fn;
    ccl_buffer in_buf;
    size_t in_cnt;
    ccl_datatype in_dtype;
    void** out_buf;
    size_t* out_cnt;
    ccl_datatype_t* out_dtype_idx;
};
