#pragma once

#include "sched/entry/coll/base_coll_entry.hpp"

class reduce_entry : public base_coll_entry
{
public:
    reduce_entry() = delete;
    reduce_entry(iccl_sched *sched,
                     const void *send_buf,
                     void *recv_buf,
                     size_t cnt,
                     iccl_datatype_internal_t dtype,
                     iccl_reduction_t reduction,
                     size_t root) :
        base_coll_entry(sched), send_buf(send_buf), recv_buf(recv_buf),
        cnt(cnt), dtype(dtype), op(reduction), root(root)
    {
        LOG_DEBUG("creating ", name(), " entry");
    }

    void start_derived()
    {
        LOG_DEBUG("REDUCE entry req ", &req, ", cnt ", cnt);
        atl_status_t atl_status = atl_comm_reduce(sched->bin->get_comm_ctx(), send_buf, recv_buf,
                                                  cnt, root, static_cast<atl_datatype_t>(dtype->type),
                                                  static_cast<atl_reduction_t>(op), &req);

        if (unlikely(atl_status != atl_status_success))
        {
            ICCL_THROW("REDUCE entry failed. atl_status: ", atl_status_to_str(atl_status));
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
            ICCL_THROW("REDUCE entry failed. atl_status: ", atl_status_to_str(atl_status));
        }

        if (req_status)
            status = iccl_sched_entry_status_complete;
    }

    const char* name() const
    {
        return "REDUCE";
    }

protected:
    void dump_detail(std::stringstream& str) const
    {
        iccl_logger::format(str,
                            "dt ", iccl_datatype_get_name(dtype),
                            ", cnt ", cnt,
                            ", send_buf ", send_buf,
                            ", recv_buf ", recv_buf,
                            ", op ", iccl_reduction_to_str(op),
                            ", root ", root,
                            ", comm_id ", sched->coll_param.comm->id(),
                            ", req ",&req,
                            "\n");
    }

private:
    const void* send_buf;
    void* recv_buf;
    size_t cnt;
    iccl_datatype_internal_t dtype;
    iccl_reduction_t op;
    size_t root;
    atl_req_t req{};
};
