#pragma once

#include "sched/entry_types/entry.hpp"
#include "sched/sched.hpp"

class copy_entry : public sched_entry
{
public:
    copy_entry() = delete;
    copy_entry(mlsl_sched* schedule,
               const void* in_buffer,
               void* out_buffer,
               size_t count,
               mlsl_datatype_internal_t data_type) :
        sched_entry(schedule), in_buf(in_buffer), out_buf(out_buffer), cnt(count), dtype(data_type)
    {}

    void execute()
    {
        if(status == mlsl_sched_entry_status_not_started)
        {
            auto elem_buf = in_buf == MLSL_POSTPONED_ADDR ? sched->postponed_fields.buf : in_buf;
            auto elem_count = cnt == MLSL_POSTPONED_COUNT ? sched->postponed_fields.count : cnt;
            auto elem_dtype = dtype == MLSL_POSTPONED_DTYPE ? &sched->postponed_fields.dtype : dtype;
            auto comp_status = mlsl_comp_copy(elem_buf, out_buf, elem_count, elem_dtype);
            MLSL_ASSERT(comp_status == mlsl_status_success);
            status = mlsl_sched_entry_status_complete;
        }
    }

    void adjust(size_t partition_idx,
                size_t partition_count)
    {
        size_t adjust_count, adjust_offset;
        get_count_and_offset(cnt, dtype, partition_idx, partition_count, adjust_count, adjust_offset);
        cnt = adjust_count;
        adjust_ptr(in_buf, adjust_offset);
        adjust_ptr(out_buf, adjust_offset);
    }

    const char* name() const
    {
        return "COPY";
    }

    std::shared_ptr<sched_entry> clone() const
    {
        //full member-wise copy
        return std::make_shared<copy_entry>(*this);
    }

protected:
    char* dump_detail(char* dump_buf) const
    {
        auto bytes_written = sprintf(dump_buf, "dt %s, cnt %zu, in_buf %p, out_buf %p\n",
                                     mlsl_datatype_get_name(dtype), cnt, in_buf, out_buf);
        return dump_buf + bytes_written;
    }

private:
    const void* in_buf;
    void* out_buf;
    size_t cnt;
    mlsl_datatype_internal_t dtype;
};
