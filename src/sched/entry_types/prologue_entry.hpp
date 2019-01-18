#pragma once

#include "sched/entry_types/entry.hpp"
#include "sched/sched.hpp"

class prologue_entry : public sched_entry
{
public:
    prologue_entry() = delete;
    prologue_entry(mlsl_prologue_fn_t prologue_fn,
                   const void* in_buffer,
                   size_t in_count,
                   mlsl_datatype_internal_t in_data_type,
                   void** out_buffer,
                   size_t* out_count,
                   mlsl_datatype_internal* out_data_type) :
        fn(prologue_fn), in_buf(in_buffer), in_cnt(in_count), in_dtype(in_data_type),
        out_buf(out_buffer), out_cnt(out_count), out_dtype(out_data_type)
    {}

    void execute()
    {
        if(status == mlsl_sched_entry_status_not_started)
        {
            mlsl_datatype_internal* elem_dtype_nonconst = out_dtype;
            fn(in_buf, in_cnt, in_dtype->type, out_buf, out_cnt, &(elem_dtype_nonconst->type),
               &elem_dtype_nonconst->size);
            status = mlsl_sched_entry_status_complete;
        }
    }

    const char* name() const
    {
        return "PROLOGUE";
    }

    std::shared_ptr<sched_entry> clone() const
    {
        //full member-wise copy
        return std::make_shared<prologue_entry>(*this);
    }

protected:
    char* dump_detail(char* dump_buf) const
    {
        auto bytes_written = sprintf(dump_buf,
                                     "in_dt %s, in_cnt %zu, in_buf %p, out_dt %s, out_cnt %p, out_buf %p, fn %p\n",
                                     mlsl_datatype_get_name(in_dtype), in_cnt, in_buf,
                                     mlsl_datatype_get_name(out_dtype), out_cnt, out_buf, fn);
        return dump_buf + bytes_written;
    }

private:
    mlsl_prologue_fn_t fn;
    const void* in_buf;
    size_t in_cnt;
    mlsl_datatype_internal_t in_dtype;
    void** out_buf;
    size_t* out_cnt;
    mlsl_datatype_internal* out_dtype;
};
