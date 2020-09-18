#pragma once

#include "sched/entry/coll/direct/base_coll_entry.hpp"

class bcast_entry : public base_coll_entry {
public:
    static constexpr const char* class_name() noexcept {
        return "BCAST";
    }

    bcast_entry() = delete;
    bcast_entry(ccl_sched* sched,
                ccl_buffer buf,
                size_t cnt,
                const ccl_datatype& dtype,
                size_t root,
                ccl_comm* comm)
            : base_coll_entry(sched),
              buf(buf),
              cnt(cnt),
              root(root),
              dtype(dtype),
              comm(comm) {
        //TODO: Add way to using MPI communicator
        CCL_UNUSED(this->comm);
    }

    void start() override {
        size_t bytes = cnt * dtype.size();
        LOG_DEBUG("BCAST entry req ", &req, ", bytes ", bytes);

        atl_status_t atl_status = comm->atl->atl_ep_bcast(
            sched->bin->get_atl_ep(), buf.get_ptr(bytes), bytes, root, &req);
        if (unlikely(atl_status != ATL_STATUS_SUCCESS)) {
            CCL_THROW("BCAST entry failed. atl_status: ", atl_status_to_str(atl_status));
        }
        else
            status = ccl_sched_entry_status_started;
    }

    void update() override {
        int req_status;
        atl_status_t atl_status =
            comm->atl->atl_ep_check(sched->bin->get_atl_ep(), &req_status, &req);

        if (unlikely(atl_status != ATL_STATUS_SUCCESS)) {
            CCL_THROW("BCAST entry failed. atl_status: ", atl_status_to_str(atl_status));
        }

        if (req_status) {
            status = ccl_sched_entry_status_complete;
        }
    }

    const char* name() const override {
        return class_name();
    }

protected:
    void dump_detail(std::stringstream& str) const override {
        ccl_logger::format(str,
                           "dt ",
                           ccl::global_data::get().dtypes->name(dtype),
                           ", cnt ",
                           cnt,
                           ", root ",
                           root,
                           ", buf ",
                           buf,
                           ", comm_id ",
                           sched->get_comm_id(),
                           ", req ",
                           &req,
                           "\n");
    }

private:
    ccl_buffer buf;
    size_t cnt;
    size_t root;
    ccl_datatype dtype;
    ccl_comm* comm;
    atl_req_t req{};
};
