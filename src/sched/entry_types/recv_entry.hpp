#pragma once

#include "sched/entry_types/entry.hpp"
#include "sched/sched_queue.hpp"

class recv_entry : public sched_entry
{
public:
    recv_entry() = delete;
    recv_entry(mlsl_sched* sched,
               void* buf,
               size_t cnt,
               mlsl_datatype_internal_t dtype,
               size_t src,
               mlsl_op_id_t op_id) :
        sched_entry(sched), buf(buf), cnt(cnt), dtype(dtype), src(src), op_id(op_id)
    {
        LOG_DEBUG("creating ", name(), " entry");
        pfields.add_available(mlsl_sched_entry_field_buf);
        pfields.add_available(mlsl_sched_entry_field_cnt);
    }

    void start_derived()
    {
        atl_tag = mlsl_create_atl_tag(sched->coll_param.comm->id(), sched->sched_id, op_id, src);
        size_t bytes = cnt * mlsl_datatype_get_size(dtype);
        LOG_DEBUG("RECV entry src ", src, ", tag ", std::setbase(16), atl_tag, ", req ", &req, ", bytes ", bytes);
        atl_status_t atl_status = atl_comm_recv(sched->bin->get_comm_ctx(), buf,
                                                bytes, src, atl_tag, &req);
        if (unlikely(atl_status != atl_status_success))
        {
            MLSL_THROW("RECV entry failed. atl_status: ", atl_status);
        }
        else
        {
            status = mlsl_sched_entry_status_started;
        }
    }

    void update_derived()
    {
        int req_status;
        atl_status_t atl_status = atl_comm_check(sched->bin->get_comm_ctx(), &req_status, &req);

        if (unlikely(atl_status != atl_status_success))
        {
            MLSL_THROW("RECV entry failed. atl_status: ", atl_status);
        }

        if (req_status)
        {
            status = mlsl_sched_entry_status_complete;
        }
    }

    void* get_field_ptr(mlsl_sched_entry_field_id id)
    {
        switch (id)
        {
            case mlsl_sched_entry_field_buf: return &buf;
            case mlsl_sched_entry_field_cnt: return &cnt;
            default: MLSL_FATAL("unexpected id ", id);
        }
    }

    const char* name() const
    {
        return "RECV";
    }

protected:
    void dump_detail(std::stringstream& str) const
    {
        mlsl_logger::format(str,
                            "dt ", mlsl_datatype_get_name(dtype),
                            ", cnt ", cnt,
                            ", buf ", buf,
                            ", src ", src,
                            ", atl_tag ", std::setbase(16), atl_tag,
                            ", comm_id ", sched->coll_param.comm->id(),
                            ", req ", &req,
                            "\n");
    }

private:
    void* buf;
    size_t cnt;
    mlsl_datatype_internal_t dtype;
    size_t src;
    mlsl_op_id_t op_id = 0;
    mlsl_atl_comm_tag_t atl_tag{};
    atl_req_t req{};
};
