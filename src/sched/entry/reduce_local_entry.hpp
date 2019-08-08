#pragma once

#include "comp/comp.hpp"
#include "sched/entry/entry.hpp"

class reduce_local_entry : public sched_entry
{
public:
    static constexpr const char* entry_class_name() noexcept
    {
        return "REDUCE_LOCAL";
    }

    reduce_local_entry() = delete;
    reduce_local_entry(ccl_sched* sched,
                       const ccl_buffer in_buf,
                       size_t in_cnt,
                       ccl_buffer inout_buf,
                       size_t* out_cnt,
                       ccl_datatype_internal_t dtype,
                       ccl_reduction_t reduction_op) :
        sched_entry(sched), in_buf(in_buf),
        in_cnt(in_cnt), inout_buf(inout_buf),
        out_cnt(out_cnt), dtype(dtype), op(reduction_op),
        fn(sched->coll_attr.reduction_fn)
    {
        CCL_THROW_IF_NOT(op != ccl_reduction_custom || fn,
                          "custom reduction requires user provided callback");
    }

    void start_derived() override
    {
        size_t bytes = in_cnt * ccl_datatype_get_size(dtype);
        ccl_status_t comp_status = ccl_comp_reduce(in_buf.get_ptr(bytes), in_cnt,
                                                     inout_buf.get_ptr(bytes), out_cnt,
                                                     dtype, op, fn);
        CCL_ASSERT(comp_status == ccl_status_success, "bad status ", comp_status);
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
                           "dt ", ccl_datatype_get_name(dtype),
                           ", in_buf ", in_buf,
                           ", in_cnt ", in_cnt,
                           ", inout_buf ", inout_buf,
                           ", out_cnt ", out_cnt,
                           ", op ", ccl_reduction_to_str(op),
                           ", red_fn ", fn,
                           "\n");
    }

private:
    ccl_buffer in_buf;
    size_t in_cnt;
    ccl_buffer inout_buf;
    size_t* out_cnt;
    ccl_datatype_internal_t dtype;
    ccl_reduction_t op;
    ccl_reduction_fn_t fn;
};
