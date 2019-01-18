#pragma once

#include "sched/entry_types/entry.hpp"
#include "sched/sched.hpp"

class postponed_fields_entry : public sched_entry
{
public:
    postponed_fields_entry() = delete;
    postponed_fields_entry(mlsl_sched* schedule,
                           size_t part_idx,
                           size_t part_count) :
        sched_entry(schedule), part_idx(part_idx), part_count(part_count)
    {}

    void execute()
    {
        if(status == mlsl_sched_entry_status_not_started)
        {
            auto elem_dtype = &sched->postponed_fields.dtype;
            auto original_count = sched->postponed_fields.count;
            sched->postponed_fields.count = original_count / part_count;
            sched->postponed_fields.buf = static_cast<char*>(sched->postponed_fields.buf) +
                                          part_idx * sched->postponed_fields.count * mlsl_datatype_get_size(elem_dtype);
            if (part_idx == (part_count - 1))
            {
                sched->postponed_fields.count += original_count % part_count;
            }
            MLSL_LOG(DEBUG, "UPDATE_FIELDS: entry, part_idx %zu, part_count %zu, dtype %s, count %zu, buf %p",
                     part_idx, part_count, mlsl_datatype_get_name(elem_dtype),
                     sched->postponed_fields.count, sched->postponed_fields.buf);
            status = mlsl_sched_entry_status_complete;
        }
    }

    const char* name() const
    {
        return "UPDATE_FIELDS";
    }

    std::shared_ptr<sched_entry> clone() const
    {
        //full member-wise copy
        return std::make_shared<postponed_fields_entry>(*this);
    }

protected:
    char* dump_detail(char* dump_buf) const
    {
        auto bytes_written = sprintf(dump_buf, "part_idx %zu, part_count %zu\n", part_idx, part_count);
        return dump_buf + bytes_written;
    }

private:
    size_t part_idx;
    size_t part_count;
};
