#pragma once

#include "sched/entry/entry.hpp"

class copy_entry : public sched_entry,
                   public postponed_fields<copy_entry,
                                           ccl_sched_entry_field_in_buf,
                                           ccl_sched_entry_field_cnt,
                                           ccl_sched_entry_field_dtype>
{
public:
    static constexpr const char* class_name() noexcept
    {
        return "COPY";
    }

    copy_entry() = delete;
    copy_entry(ccl_sched* sched,
               const ccl_buffer in_buf,
               ccl_buffer out_buf,
               size_t cnt,
               const ccl_datatype& dtype) :
        sched_entry(sched), in_buf(in_buf),
        out_buf(out_buf), cnt(cnt), dtype(dtype)
    {
    }

    void start() override
    {
        update_fields();

        size_t bytes = cnt * dtype.size();
        auto comp_status = ccl_comp_copy(in_buf.get_ptr(bytes), out_buf.get_ptr(bytes), cnt, dtype);
        CCL_ASSERT(comp_status == ccl_status_success, "bad status ", comp_status);
        status = ccl_sched_entry_status_complete;
    }

    const char* name() const override
    {
        return class_name();
    }

    ccl_buffer& get_field_ref(field_id_t<ccl_sched_entry_field_in_buf> id)
    {
        return in_buf;
    }

    size_t& get_field_ref(field_id_t<ccl_sched_entry_field_cnt> id)
    {
        return cnt;
    }

    ccl_datatype& get_field_ref(field_id_t<ccl_sched_entry_field_dtype> id)
    {
        return dtype;
    }

protected:
    void dump_detail(std::stringstream& str) const override
    {
        ccl_logger::format(str,
                           "dt ", global_data.dtypes->name(dtype),
                           ", cnt ", cnt,
                           ", in_buf ", in_buf,
                           ", out_buf ", out_buf,
                           "\n");
    }

private:
    ccl_buffer in_buf;
    ccl_buffer out_buf;
    size_t cnt;
    ccl_datatype dtype;
};
