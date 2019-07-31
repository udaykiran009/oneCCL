#pragma once

#include "sched/entry/entry.hpp"

class copy_entry : public sched_entry
{
public:
    static constexpr const char *entry_class_name() noexcept
    {
        return "COPY";
    }

    copy_entry() = delete;
    copy_entry(ccl_sched* sched,
               const ccl_buffer in_buf,
               ccl_buffer out_buf,
               size_t cnt,
               ccl_datatype_internal_t dtype) :
        sched_entry(sched), in_buf(in_buf), out_buf(out_buf), cnt(cnt), dtype(dtype)
    {
        pfields.add_available(ccl_sched_entry_field_in_buf);
        pfields.add_available(ccl_sched_entry_field_cnt);
        pfields.add_available(ccl_sched_entry_field_dtype);
    }

    void start_derived() override
    {
        size_t bytes = cnt * ccl_datatype_get_size(dtype);
        auto comp_status = ccl_comp_copy(in_buf.get_ptr(bytes), out_buf.get_ptr(bytes), cnt, dtype);
        CCL_ASSERT(comp_status == ccl_status_success, "bad status ", comp_status);
        status = ccl_sched_entry_status_complete;
    }

    void* get_field_ptr(ccl_sched_entry_field_id id) override
    {
        switch (id)
        {
            case ccl_sched_entry_field_in_buf: return &in_buf;
            case ccl_sched_entry_field_cnt: return &cnt;
            case ccl_sched_entry_field_dtype: return &dtype;
            default: CCL_FATAL("unexpected id ", id);
        }
        return nullptr;
    }

    const char* name() const override
    {
        return entry_class_name();
    }

protected:
    void dump_detail(std::stringstream& str) const override
    {
        ccl_logger::format(str,
                           "dt ", ccl_datatype_get_name(dtype),
                           ", cnt ", cnt,
                           ", in_buf ", in_buf,
                           ", out_buf ", out_buf,
                           "\n");
    }

private:
    ccl_buffer in_buf;
    ccl_buffer out_buf;
    size_t cnt;
    ccl_datatype_internal_t dtype;
};
