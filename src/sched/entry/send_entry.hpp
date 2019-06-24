#pragma once

#include "common/global/global.hpp"
#include "sched/entry/entry.hpp"
#include "sched/sched_queue.hpp"

class send_entry : public sched_entry
{
public:
    send_entry() = delete;
    send_entry(mlsl_sched* sched,
               const void* buffer,
               size_t count,
               mlsl_datatype_internal_t dtype,
               size_t dst,
               size_t rank,
               mlsl_op_id_t op_id) :
        sched_entry(sched), buf(buffer),
        cnt(count), dtype(dtype),
        dst(dst), rank(rank), op_id(op_id)
    {
        LOG_DEBUG("creating ", name(), " entry");
        pfields.add_available(mlsl_sched_entry_field_buf);
        pfields.add_available(mlsl_sched_entry_field_cnt);
    }

    void start_derived()
    {
        atl_tag = global_data.atl_tag->create(sched->coll_param.comm->id(), rank, sched->sched_id, op_id);
        size_t bytes = cnt * mlsl_datatype_get_size(dtype);
        LOG_DEBUG("SEND entry dst, ", dst, ", tag ", atl_tag, ", req ", &req, ", bytes ", bytes);
        atl_status_t atl_status = atl_comm_send(sched->bin->get_comm_ctx(), buf,
                                                bytes, dst, atl_tag, &req);
        if (unlikely(atl_status != atl_status_success))
        {
            MLSL_THROW("SEND entry failed. atl_status: ", atl_status_to_str(atl_status));
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
            MLSL_THROW("SEND entry failed. atl_status: ", atl_status_to_str(atl_status));
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
            default: MLSL_FATAL("unexpected  ", id);
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
        mlsl_logger::format(str,
                            "dt ", mlsl_datatype_get_name(dtype),
                            ", cnt ", cnt,
                            ", buf ", buf,
                            ", dst ", dst,
                            ", atl_tag ", atl_tag,
                            ", comm_id ", sched->coll_param.comm->id(),
                            ", req ", &req,
                            "\n");
    }

private:
    const void* buf;
    size_t cnt;
    mlsl_datatype_internal_t dtype;
    //destination in global communicator
    size_t dst;
    //its rank in global communicator
    size_t rank;
    mlsl_op_id_t op_id = 0;
    atl_req_t req{};
    uint64_t atl_tag{};
};
