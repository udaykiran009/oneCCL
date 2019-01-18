#pragma once

#include "sched/entry_types/entry.hpp"
#include "sched/sched.hpp"
#include "comp/comp.hpp"

class reduce_entry : public sched_entry
{
public:
    reduce_entry() = delete;
    reduce_entry(const void* in_buffer,
                 size_t in_count,
                 void* inout_buffer,
                 size_t* out_count,
                 mlsl_datatype_internal_t data_type,
                 mlsl_reduction_t reduction_op,
                 mlsl_reduction_fn_t reduction_fn) :
        in_buf(in_buffer), in_cnt(in_count), inout_buf(inout_buffer), out_cnt(out_count), dtype(data_type),
        op(reduction_op), fn(reduction_fn)
    {
        MLSL_ASSERT_FMT(op != mlsl_reduction_custom || fn, "Custom reduction requires user provided callback");
    }

    void execute()
    {
        if(status == mlsl_sched_entry_status_not_started)
        {
            mlsl_status_t comp_status = mlsl_comp_reduce(in_buf, in_cnt, inout_buf, out_cnt, dtype, op, fn);
            MLSL_ASSERT(comp_status == mlsl_status_success);
            status = mlsl_sched_entry_status_complete;
        }
    }

    void adjust(size_t partition_idx,
                size_t partition_count)
    {
        size_t adjust_count, adjust_offset;
        get_count_and_offset(in_cnt, dtype, partition_idx, partition_count, adjust_count, adjust_offset);
        in_cnt = adjust_count;
        adjust_ptr(in_buf, adjust_offset);
        adjust_ptr(inout_buf, adjust_offset);
    }

    const char* name() const
    {
        return "REDUCE";
    }

    std::shared_ptr<sched_entry> clone() const
    {
        //full member-wise copy
        return std::make_shared<reduce_entry>(*this);
    }

protected:
    char* dump_detail(char* dump_buf) const
    {
        auto bytes_written = sprintf(dump_buf,
                                     "dt %s, in_buf %p, in_cnt %zu, inout_buf %p, out_cnt %p, op %s, red_fn %p\n",
                                     mlsl_datatype_get_name(dtype), in_buf, in_cnt, inout_buf, out_cnt,
                                     mlsl_reduction_to_str(op), fn);

        return dump_buf + bytes_written;
    }

private:
    const void* in_buf;
    size_t in_cnt;
    void* inout_buf;
    size_t* out_cnt;
    mlsl_datatype_internal_t dtype;
    mlsl_reduction_t op;
    mlsl_reduction_fn_t fn;
};
