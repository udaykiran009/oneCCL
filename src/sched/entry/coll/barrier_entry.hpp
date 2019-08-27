#pragma once

#include "sched/entry/coll/base_coll_entry.hpp"

class barrier_entry : public base_coll_entry
{
public:
    static constexpr const char* class_name() noexcept
    {
        return "BARRIER";
    }

    barrier_entry() = delete;
    barrier_entry(ccl_sched* sched) :
        base_coll_entry(sched)
    {
    }

    void start_derived() override
    {
        LOG_DEBUG("BARRIER entry req ", &req);

        atl_status_t atl_status = atl_comm_barrier(sched->bin->get_comm_ctx(), &req);
        if (unlikely(atl_status != atl_status_success))
        {
            CCL_THROW("BARRIER entry failed. atl_status: ", atl_status_to_str(atl_status));
        }
        else
            status = ccl_sched_entry_status_started;
    }

    void update_derived() override
    {
        int req_status;
        atl_status_t atl_status = atl_comm_check(sched->bin->get_comm_ctx(), &req_status, &req);

        if (unlikely(atl_status != atl_status_success))
        {
            CCL_THROW("BARRIER entry failed. atl_status: ", atl_status_to_str(atl_status));
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
                            "comm_id ", sched->coll_param.comm->id(),
                            ", req ",&req,
                            "\n");
    }

private:
    atl_req_t req{};
};
