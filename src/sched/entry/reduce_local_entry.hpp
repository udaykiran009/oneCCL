#pragma once

#include "comp/comp.hpp"
#include "sched/entry/entry.hpp"

class reduce_local_entry : public sched_entry
{
public:
    reduce_local_entry() = delete;
    reduce_local_entry(iccl_sched* sched,
                       const void* in_buf,
                       size_t in_cnt,
                       void* inout_buf,
                       size_t* out_cnt,
                       iccl_datatype_internal_t dtype,
                       iccl_reduction_t reduction_op) :
        sched_entry(sched), in_buf(in_buf),
        in_cnt(in_cnt), inout_buf(inout_buf),
        out_cnt(out_cnt), dtype(dtype), op(reduction_op),
        fn(sched->coll_attr.reduction_fn)
    {
        LOG_DEBUG("creating ", name(), " entry");
        ICCL_THROW_IF_NOT(op != iccl_reduction_custom || fn,
                          "custom reduction requires user provided callback");
    }

    void start_derived()
    {
        iccl_status_t comp_status = iccl_comp_reduce(in_buf, in_cnt, inout_buf, out_cnt,
                                                     dtype, op, fn);
        ICCL_ASSERT(comp_status == iccl_status_success, "bad status ", comp_status);
        status = iccl_sched_entry_status_complete;
    }

    const char* name() const
    {
        return "REDUCE_LOCAL";
    }

protected:
    void dump_detail(std::stringstream& str) const
    {
        iccl_logger::format(str,
                            "dt ", iccl_datatype_get_name(dtype),
                            ", in_buf ", in_buf,
                            ", in_cnt ", in_cnt,
                            ", inout_buf ", inout_buf,
                            ", out_cnt ", out_cnt,
                            ", op ", iccl_reduction_to_str(op),
                            ", red_fn ", fn,
                            "\n");
    }

private:
    const void* in_buf;
    size_t in_cnt;
    void* inout_buf;
    size_t* out_cnt;
    iccl_datatype_internal_t dtype;
    iccl_reduction_t op;
    iccl_reduction_fn_t fn;
};
