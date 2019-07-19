#pragma once

#include "common/global/global.hpp"
#include "sched/entry/entry.hpp"
#include "sched/sched_queue.hpp"

class send_entry : public sched_entry
{
public:
    send_entry() = delete;
    send_entry(iccl_sched* sched,
               const iccl_buffer buf,
               size_t cnt,
               iccl_datatype_internal_t dtype,
               size_t dst,
               size_t rank,
               iccl_op_id_t op_id) :
        sched_entry(sched), buf(buf),
        cnt(cnt), dtype(dtype),
        dst(dst), rank(rank), op_id(op_id)
    {
        LOG_DEBUG("creating ", name(), " entry");
        pfields.add_available(iccl_sched_entry_field_buf);
        pfields.add_available(iccl_sched_entry_field_cnt);
    }

    void start_derived()
    {
        atl_tag = global_data.atl_tag->create(sched->coll_param.comm->id(), rank, sched->sched_id, op_id);
        size_t bytes = cnt * iccl_datatype_get_size(dtype);
        LOG_DEBUG("SEND entry dst, ", dst, ", tag ", atl_tag, ", req ", &req, ", bytes ", bytes);

        atl_status_t atl_status = atl_comm_send(sched->bin->get_comm_ctx(), buf.get_ptr(bytes),
                                                bytes, dst, atl_tag, &req);

        if (unlikely(atl_status != atl_status_success))
        {
            ICCL_THROW("SEND entry failed. atl_status: ", atl_status_to_str(atl_status));
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
            ICCL_THROW("SEND entry failed. atl_status: ", atl_status_to_str(atl_status));
        }

        if (req_status)
        {
            LOG_DEBUG("SEND entry done dst, ", dst);
            status = iccl_sched_entry_status_complete;
        }
    }

    void* get_field_ptr(iccl_sched_entry_field_id id)
    {
        switch (id)
        {
            case iccl_sched_entry_field_buf: return &buf;
            case iccl_sched_entry_field_cnt: return &cnt;
            default: ICCL_FATAL("unexpected  ", id);
        }
        return nullptr;
    }

    const char* name() const
    {
        return "SEND";
    }

protected:
    void dump_detail(std::stringstream& str) const
    {
        iccl_logger::format(str,
                            "dt ", iccl_datatype_get_name(dtype),
                            ", cnt ", cnt,
                            ", buf ", buf,
                            ", dst ", dst,
                            ", atl_tag ", atl_tag,
                            ", comm_id ", sched->coll_param.comm->id(),
                            ", req ", &req,
                            "\n");
    }

private:
    iccl_buffer buf;
    size_t cnt;
    iccl_datatype_internal_t dtype;
    size_t dst;
    size_t rank;
    iccl_op_id_t op_id = 0;
    atl_req_t req{};
    uint64_t atl_tag{};
};
