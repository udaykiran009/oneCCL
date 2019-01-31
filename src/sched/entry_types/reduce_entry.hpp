#pragma once

#include "sched/entry_types/entry.hpp"
#include "comp/comp.hpp"

class reduce_entry : public sched_entry
{
public:
    reduce_entry() = delete;
    reduce_entry(mlsl_sched* sched,
                 const void* in_buf,
                 size_t in_cnt,
                 void* inout_buf,
                 size_t* out_cnt,
                 mlsl_datatype_internal_t dtype,
                 mlsl_reduction_t reduction_op) :
        sched_entry(sched), in_buf(in_buf),
        in_cnt(in_cnt), inout_buf(inout_buf),
        out_cnt(out_cnt), dtype(dtype), op(reduction_op)
    {
        MLSL_ASSERTP_FMT(op != mlsl_reduction_custom || sched->coll_attr.reduction_fn,
            "custom reduction requires user provided callback");
    }

    void start_derived()
    {
        mlsl_status_t comp_status = mlsl_comp_reduce(in_buf, in_cnt, inout_buf, out_cnt,
                                                     dtype, op, sched->coll_attr.reduction_fn);
        MLSL_ASSERT(comp_status == mlsl_status_success);
        status = mlsl_sched_entry_status_complete;
    }

    const char* name() const
    {
        return "REDUCE";
    }

protected:
    char* dump_detail(char* dump_buf) const
    {
        auto bytes_written = sprintf(dump_buf,
                                     "dt %s, in_buf %p, in_cnt %zu, inout_buf %p, out_cnt %p, op %s, red_fn %p\n",
                                     mlsl_datatype_get_name(dtype), in_buf, in_cnt, inout_buf, out_cnt,
                                     mlsl_reduction_to_str(op), sched->coll_attr.reduction_fn);

        return dump_buf + bytes_written;
    }

private:
    const void* in_buf;
    size_t in_cnt;
    void* inout_buf;
    size_t* out_cnt;
    mlsl_datatype_internal_t dtype;
    mlsl_reduction_t op;
};
