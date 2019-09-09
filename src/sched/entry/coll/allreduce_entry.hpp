#pragma once

#include "sched/entry/coll/base_coll_entry.hpp"

class allreduce_entry : public base_coll_entry
{
public:
    static constexpr const char* class_name() noexcept
    {
        return "ALLREDUCE";
    }

    allreduce_entry() = delete;
    allreduce_entry(ccl_sched* sched,
                    const ccl_buffer send_buf,
                    ccl_buffer recv_buf,
                    size_t cnt,
                    ccl_datatype_internal_t dtype,
                    ccl_reduction_t op) :
        base_coll_entry(sched), send_buf(send_buf), recv_buf(recv_buf),
        cnt(cnt), dtype(dtype), op(op)
    {
    }

    void start() override
    {
        LOG_DEBUG("ALLREDUCE entry req ", &req, ", cnt ", cnt);
        size_t bytes = cnt * ccl_datatype_get_size(dtype);
        atl_status_t atl_status = atl_comm_allreduce(sched->bin->get_comm_ctx(), send_buf.get_ptr(bytes),
                                                     recv_buf.get_ptr(bytes), cnt,
                                                     static_cast<atl_datatype_t>(dtype->type),
                                                     static_cast<atl_reduction_t>(op), &req);
        if (unlikely(atl_status != atl_status_success))
        {
            CCL_THROW("ALLREDUCE entry failed. atl_status: ", atl_status_to_str(atl_status));
        }
        else
            status = ccl_sched_entry_status_started;
    }

    void update() override
    {
        int req_status;
        atl_status_t atl_status = atl_comm_check(sched->bin->get_comm_ctx(), &req_status, &req);

        if (unlikely(atl_status != atl_status_success))
        {
            CCL_THROW("ALLREDUCE entry failed. atl_status: ", atl_status_to_str(atl_status));
        }

        if (req_status)
            status = ccl_sched_entry_status_complete;
    }

    const char* name() const override
    {
        return class_name();
    }

protected:
    void dump_detail(std::stringstream& str) const override
    {
        ccl_logger::format(str,
                            "dt ", ccl_datatype_get_name(dtype),
                            ", cnt ", cnt,
                            ", send_buf ", send_buf,
                            ", recv_buf ", recv_buf,
                            ", op ", ccl_reduction_to_str(op),
                            ", comm_id ", sched->coll_param.comm->id(),
                            ", req ",&req,
                            "\n");
    }

private:
    ccl_buffer send_buf;
    ccl_buffer recv_buf;
    size_t cnt;
    ccl_datatype_internal_t dtype;
    ccl_reduction_t op;
    atl_req_t req{};
};
