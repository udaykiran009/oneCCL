#pragma once

#include "common/global/global.hpp"
#include "sched/entry/entry.hpp"
#include "sched/sched_queue.hpp"

class recv_entry : public sched_entry
{
public:
    recv_entry() = delete;
    recv_entry(iccl_sched* sched,
               void* buf,
               size_t cnt,
               iccl_datatype_internal_t dtype,
               size_t src,
               iccl_op_id_t op_id) :
        sched_entry(sched), buf(buf), cnt(cnt), dtype(dtype), src(src), op_id(op_id)
    {
        LOG_DEBUG("creating ", name(), " entry");
        pfields.add_available(iccl_sched_entry_field_buf);
        pfields.add_available(iccl_sched_entry_field_cnt);
    }

    void start_derived()
    {
        atl_tag = global_data.atl_tag->create(sched->coll_param.comm->id(), src, sched->sched_id, op_id);
        size_t bytes = cnt * iccl_datatype_get_size(dtype);
        LOG_DEBUG("RECV entry src ", src, ", tag ", atl_tag, ", req ", &req, ", bytes ", bytes);
        atl_status_t atl_status = atl_comm_recv(sched->bin->get_comm_ctx(), buf,
                                                bytes, src, atl_tag, &req);
        if (unlikely(atl_status != atl_status_success))
        {
            ICCL_THROW("RECV entry failed. atl_status: ", atl_status_to_str(atl_status));
        }
        else
        {
            status = iccl_sched_entry_status_started;
        }
    }

    void update_derived()
    {
        int req_status;
        atl_status_t atl_status = atl_comm_check(sched->bin->get_comm_ctx(), &req_status, &req);

        if (unlikely(atl_status != atl_status_success))
        {
            ICCL_THROW("RECV entry failed. atl_status: ", atl_status_to_str(atl_status));
        }

        if (req_status)
        {
            status = iccl_sched_entry_status_complete;
        }
    }

    void* get_field_ptr(iccl_sched_entry_field_id id)
    {
        switch (id)
        {
            case iccl_sched_entry_field_buf: return &buf;
            case iccl_sched_entry_field_cnt: return &cnt;
            default: ICCL_FATAL("unexpected id ", id);
        }
    }

    const char* name() const
    {
        return "RECV";
    }

protected:
    void dump_detail(std::stringstream& str) const
    {
        iccl_logger::format(str,
                            "dt ", iccl_datatype_get_name(dtype),
                            ", cnt ", cnt,
                            ", buf ", buf,
                            ", src ", src,
                            ", atl_tag ", atl_tag,
                            ", comm_id ", sched->coll_param.comm->id(),
                            ", req ", &req,
                            "\n");
    }

private:
    void* buf;
    size_t cnt;
    iccl_datatype_internal_t dtype;
    size_t src;
    iccl_op_id_t op_id = 0;
    uint64_t atl_tag{};
    atl_req_t req{};
};
