#pragma once

#include "sched/entry_types/entry.hpp"
#include "sched/sched.hpp"

class epilogue_entry : public sched_entry
{
public:
    epilogue_entry() = delete;
    epilogue_entry(mlsl_sched* schedule,
                   mlsl_epilogue_fn_t epilogue_fn,
                   const void* in_buffer,
                   size_t in_count,
                   mlsl_datatype_internal_t in_data_type,
                   void* out_buffer,
                   size_t* out_count,
                   size_t expected_out_count,
                   mlsl_datatype_internal* out_data_type) :
        sched_entry(schedule), fn(epilogue_fn), in_buf(in_buffer), in_cnt(in_count), in_dtype(in_data_type),
        out_buf(out_buffer), out_cnt(out_count), expected_out_cnt(expected_out_count), out_dtype(out_data_type)
    {}

    void execute()
    {
        if(status == mlsl_sched_entry_status_not_started)
        {
            const void* elem_buf = in_buf == MLSL_POSTPONED_ADDR ? sched->postponed_fields.buf : in_buf;
            size_t elem_count = in_cnt == MLSL_POSTPONED_COUNT ? sched->postponed_fields.count : in_cnt;
            mlsl_datatype_internal_t elem_dtype =
                in_dtype == MLSL_POSTPONED_DTYPE ? &sched->postponed_fields.dtype : in_dtype;
            fn(elem_buf, elem_count, elem_dtype->type, out_buf, out_cnt, out_dtype->type);
            MLSL_ASSERTP(expected_out_cnt == *out_cnt);
            status = mlsl_sched_entry_status_complete;
        }
    }

    const char* name() const
    {
        return "EPILOGUE";
    }

    std::shared_ptr<sched_entry> clone() const
    {
        //full member-wise copy
        return std::make_shared<epilogue_entry>(*this);
    }

protected:
    char* dump_detail(char* dump_buf) const
    {
        auto bytes_written = sprintf(dump_buf,
                                     "in_dt %s, in_cnt %zu, in_buf %p, out_dt %s, out_cnt %p, out_buf %p, fn %p, exp_out_count %zu\n",
                                     mlsl_datatype_get_name(in_dtype), in_cnt, in_buf,
                                     mlsl_datatype_get_name(out_dtype),
                                     out_cnt, out_buf, fn, expected_out_cnt);
        return dump_buf + bytes_written;
    }

private:
    mlsl_epilogue_fn_t fn;
    const void* in_buf;
    size_t in_cnt;
    mlsl_datatype_internal_t in_dtype;
    void* out_buf;
    size_t* out_cnt;
    size_t expected_out_cnt;
    mlsl_datatype_internal* out_dtype;
};
