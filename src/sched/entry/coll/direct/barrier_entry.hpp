#pragma once

#include "sched/entry/coll/direct/base_coll_entry.hpp"

class barrier_entry : public base_coll_entry {
public:
    static constexpr const char* class_name() noexcept {
        return "BARRIER";
    }

    barrier_entry() = delete;
    barrier_entry(ccl_sched* sched, ccl_comm* comm) : base_coll_entry(sched), comm(comm) {
        //TODO: Add way to using MPI communicator
        CCL_UNUSED(this->comm);
    }

    void start() override {
        LOG_DEBUG("BARRIER entry req ", &req);

        atl_status_t atl_status = comm->get_atl()->barrier(sched->bin->get_atl_ep(), &req);
        if (unlikely(atl_status != ATL_STATUS_SUCCESS)) {
            CCL_THROW("BARRIER entry failed. atl_status: ", atl_status_to_str(atl_status));
        }
        else
            status = ccl_sched_entry_status_started;
    }

    void update() override {
        atl_status_t atl_status = comm->get_atl()->check(sched->bin->get_atl_ep(), &req);

        if (unlikely(atl_status != ATL_STATUS_SUCCESS)) {
            CCL_THROW("BARRIER entry failed. atl_status: ", atl_status_to_str(atl_status));
        }

        if (req.is_completed)
            status = ccl_sched_entry_status_complete;
    }

    const char* name() const override {
        return class_name();
    }

protected:
    void dump_detail(std::stringstream& str) const override {
        ccl_logger::format(str, "comm_id ", sched->get_comm_id(), ", req ", &req, "\n");
    }

private:
    ccl_comm* comm;
    atl_req_t req{};
};
