#pragma once

#include "sched/entry/coll/base_coll_entry.hpp"

class allgatherv_entry : public base_coll_entry
{
public:
    allgatherv_entry() = delete;
    allgatherv_entry(iccl_sched* sched,
                     iccl_buf_placeholder send_buf,
                     size_t send_cnt,
                     iccl_buf_placeholder recv_buf,
                     size_t* recv_cnts,
                     iccl_datatype_internal_t dtype) :
        base_coll_entry(sched), send_buf(send_buf), send_cnt(send_cnt),
        recv_buf(recv_buf), recv_cnts(recv_cnts), dtype(dtype)
    {
        LOG_DEBUG("creating ", name(), " entry");
    }

    void start_derived()
    {
        size_t dt_size = iccl_datatype_get_size(dtype);
        size_t send_bytes = send_cnt * dt_size;
        size_t comm_size = sched->coll_param.comm->size();
        size_t i;

        recv_bytes = static_cast<int*>(ICCL_MALLOC(comm_size * sizeof(int), "recv_bytes"));
        offsets = static_cast<int*>(ICCL_MALLOC(comm_size * sizeof(int), "offsets"));

        recv_bytes[0] = recv_cnts[0] * dt_size;
        offsets[0] = 0;
        for (i = 1; i < comm_size; i++)
        {
            recv_bytes[i] = recv_cnts[i] * dt_size;
            offsets[i] = offsets[i - 1] + recv_cnts[i - 1];
        }
        LOG_DEBUG("ALLGATHERV entry req ", &req, ", send_bytes ", send_bytes);
        atl_status_t atl_status = atl_comm_allgatherv(sched->bin->get_comm_ctx(), send_buf.get_ptr(), send_bytes,
                                                      recv_buf.get_ptr(), recv_bytes, offsets, &req);

        if (unlikely(atl_status != atl_status_success))
        {
            ICCL_THROW("ALLGATHERV entry failed. atl_status: ", atl_status_to_str(atl_status));
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
            ICCL_THROW("ALLGATHERV entry failed. atl_status: ", atl_status_to_str(atl_status));
        }

        if (req_status)
        {
            status = iccl_sched_entry_status_complete;
            ICCL_FREE(recv_bytes);
            ICCL_FREE(offsets);
        }
    }

    const char* name() const
    {
        return "ALLGATHERV";
    }

protected:
    void dump_detail(std::stringstream& str) const
    {
        iccl_logger::format(str,
                            "dt ", iccl_datatype_get_name(dtype),
                            ", send_cnt ", send_cnt,
                            ", send_buf ", send_buf,
                            ", recv_cnt ", recv_cnts,
                            ", recv_buf ", recv_buf,
                            ", recv_bytes ", recv_bytes,
                            ", offsets ", offsets,
                            ", comm_id ", sched->coll_param.comm->id(),
                            ", req ",&req,
                            "\n");
    }

private:
    iccl_buf_placeholder send_buf;
    size_t send_cnt;
    iccl_buf_placeholder recv_buf;
    size_t* recv_cnts;
    int* recv_bytes;
    int* offsets;
    iccl_datatype_internal_t dtype;
    atl_req_t req{};
};
