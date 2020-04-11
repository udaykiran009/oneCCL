#pragma once

#include "sched/entry/coll/direct/base_coll_entry.hpp"

class alltoall_entry : public base_coll_entry
{
public:
    static constexpr const char* class_name() noexcept
    {
        return "ALLTOALL";
    }

    alltoall_entry() = delete;
    alltoall_entry(ccl_sched* sched,
                   const ccl_buffer send_buf,
                   ccl_buffer recv_buf,
                   size_t cnt,
                   const ccl_datatype& dtype,
                   ccl_comm* comm) :
        base_coll_entry(sched), send_buf(send_buf),
        recv_buf(recv_buf), cnt(cnt), dtype(dtype),
        comm(comm)
    {
    }

    void start() override
    {
        size_t dt_size = dtype.size();
        bytes = cnt * dt_size;

        LOG_DEBUG("ALLTOALL entry req ", &req, ", bytes ", bytes);
        atl_status_t atl_status = atl_ep_alltoall(sched->bin->get_atl_ep(),
                                                  send_buf.get_ptr(bytes),
                                                  recv_buf.get_ptr(bytes),
                                                  bytes,
                                                  &req);

        if (unlikely(atl_status != ATL_STATUS_SUCCESS))
        {
            CCL_THROW("ALLTOALL entry failed. atl_status: ", atl_status_to_str(atl_status));
        }
        else
            status = ccl_sched_entry_status_started;
    }

    void update() override
    {
        int req_status;
        atl_status_t atl_status = atl_ep_check(sched->bin->get_atl_ep(), &req_status, &req);

        if (unlikely(atl_status != ATL_STATUS_SUCCESS))
        {
            CCL_THROW("ALLTOALL entry failed. atl_status: ", atl_status_to_str(atl_status));
        }

        if (req_status)
        {
            status = ccl_sched_entry_status_complete;
        }
    }

    const char* name() const override
    {
        return class_name();
    }

protected:
    void dump_detail(std::stringstream& str) const override
    {
        ccl_logger::format(str,
                           "dt ", global_data.dtypes->name(dtype),
                           ", send_buf ", send_buf,
                           ", recv_buf ", recv_buf,
                           ", cnt ", cnt,
                           ", recv_bytes ", bytes,
                           ", comm_id ", sched->get_comm_id(),
                           ", req ",&req,
                           "\n");
    }

private:
    ccl_buffer send_buf;
    ccl_buffer recv_buf;
    size_t cnt;
    int bytes;
    ccl_datatype dtype;
    ccl_comm* comm;
    atl_req_t req{};
};
