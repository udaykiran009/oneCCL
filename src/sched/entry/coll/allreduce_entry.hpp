#pragma once

#include "sched/entry/coll/base_coll_entry.hpp"

class allreduce_entry : public base_coll_entry
{
public:
    allreduce_entry() = delete;
    allreduce_entry(mlsl_sched* sched,
                    const void* send_buf,
                    void* recv_buf,
                    size_t cnt,
                    mlsl_datatype_internal_t dtype,
                    mlsl_reduction_t op) :
        base_coll_entry(sched), send_buf(send_buf), recv_buf(recv_buf),
        cnt(cnt), dtype(dtype), op(op)
    {
        LOG_DEBUG("creating ", name(), " entry");
    }

    void start_derived()
    {
        LOG_DEBUG("ALLREDUCE entry req ", &req, ", cnt ", cnt);
        atl_status_t atl_status = atl_comm_allreduce(sched->bin->get_comm_ctx(), send_buf, recv_buf,
                                                     cnt, static_cast<atl_datatype_t>(dtype->type),
                                                     static_cast<atl_reduction_t>(op), &req);
        if (unlikely(atl_status != atl_status_success))
        {
            MLSL_THROW("ALLREDUCE entry failed. atl_status: ", atl_status_to_str(atl_status));
        }
        else
            status = mlsl_sched_entry_status_started;
    }

    void update_derived()
    {
        int req_status;
        atl_status_t atl_status = atl_comm_check(sched->bin->get_comm_ctx(), &req_status, &req);

        if (unlikely(atl_status != atl_status_success))
        {
            MLSL_THROW("ALLREDUCE entry failed. atl_status: ", atl_status_to_str(atl_status));
        }

        if (req_status)
            status = mlsl_sched_entry_status_complete;
    }

    const char* name() const
    {
        return "ALLREDUCE";
    }

protected:
    void dump_detail(std::stringstream& str) const
    {
        mlsl_logger::format(str,
                            "dt ", mlsl_datatype_get_name(dtype),
                            ", cnt ", cnt,
                            ", send_buf ", send_buf,
                            ", recv_buf ", recv_buf,
                            ", op ", mlsl_reduction_to_str(op),
                            ", comm_id ", sched->coll_param.comm->id(),
                            ", req ",&req,
                            "\n");
    }

private:
    const void* send_buf;
    void* recv_buf;
    size_t cnt;
    mlsl_datatype_internal_t dtype;
    mlsl_reduction_t op;
    atl_req_t req{};
};
