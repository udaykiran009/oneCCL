#pragma once

#include "sched/entry/entry.hpp"
#include "sched/sched_queue.hpp"

class write_entry : public sched_entry
{
public:
    write_entry() = delete;
    write_entry(iccl_sched* sched,
                iccl_buffer src_buf,
                atl_mr_t* src_mr,
                size_t cnt,
                iccl_datatype_internal_t dtype,
                size_t dst,
                atl_mr_t* dst_mr,
                size_t dst_buf_off) :
        sched_entry(sched), src_buf(src_buf), src_mr(src_mr),
        cnt(cnt), dtype(dtype), dst(dst), dst_mr(dst_mr),
        dst_buf_off(dst_buf_off)
    {
        LOG_DEBUG("creating ", name(), " entry");
        pfields.add_available(iccl_sched_entry_field_src_mr);
        pfields.add_available(iccl_sched_entry_field_dst_mr);
    }

    void start_derived()
    {
        LOG_DEBUG("WRITE entry dst ", dst, ", req ", &req);

        ICCL_THROW_IF_NOT(src_buf && src_mr && dst_mr, "incorrect values");

        if (!cnt)
        {
            status = iccl_sched_entry_status_complete;
            return;
        }

        size_t bytes = cnt * iccl_datatype_get_size(dtype);
        atl_status_t atl_status = atl_comm_write(sched->bin->get_comm_ctx(), src_buf.get_ptr(bytes),
                                                 bytes, src_mr,
                                                 (uint64_t)dst_mr->buf + dst_buf_off,
                                                 dst_mr->r_key, dst, &req);
        if (unlikely(atl_status != atl_status_success))
        {
            ICCL_THROW("WRITE entry failed. atl_status: ", atl_status_to_str(atl_status));
        }
        else
            status = iccl_sched_entry_status_started;
    }

    void update_derived()
    {
        int req_status;
        atl_status_t atl_status = atl_comm_check(sched->bin->get_comm_ctx(), &req_status, &req);

        if (unlikely(atl_status != atl_status_success))
        {
            ICCL_THROW("WRITE entry failed. atl_status: ", atl_status_to_str(atl_status));
        }

        if (req_status)
            status = iccl_sched_entry_status_complete;
    }

    void* get_field_ptr(iccl_sched_entry_field_id id)
    {
        switch (id)
        {
            case iccl_sched_entry_field_src_mr: return &src_mr;
            case iccl_sched_entry_field_dst_mr: return &dst_mr;
            default: ICCL_FATAL("unexpected id ", id);
        }
        return nullptr;
    }

    const char* name() const
    {
        return "WRITE";
    }

protected:
    void dump_detail(std::stringstream& str) const
    {
        iccl_logger::format(str,
                            "dt ", iccl_datatype_get_name(dtype),
                            ", cnt ", cnt,
                            ", src_buf ", src_buf,
                            ", src_mr ", src_mr,
                            ", dst ", dst,
                            ", dst_mr ", dst_mr,
                            ", dst_off ", dst_buf_off,
                            ", comm_id ", sched->coll_param.comm->id(),
                            ", req %p", &req,
                            "\n");
    }

private:
    iccl_buffer src_buf;
    atl_mr_t* src_mr;
    size_t cnt;
    iccl_datatype_internal_t dtype;
    size_t dst;
    atl_mr_t* dst_mr;
    size_t dst_buf_off;
    atl_req_t req{};
};
