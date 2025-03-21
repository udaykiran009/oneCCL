#pragma once

#include "sched/entry/entry.hpp"
#include "sched/sched.hpp"
#include "sched/queue/queue.hpp"

class probe_entry : public sched_entry {
public:
    static constexpr const char* class_name() noexcept {
        return "PROBE";
    }

    probe_entry() = delete;
    probe_entry(ccl_sched* sched, int src, size_t* recv_len, ccl_comm* comm)
            : sched_entry(sched),
              src(src),
              recv_len(recv_len),
              comm(comm) {}

    void start() override {
        atl_tag = comm->get_atl_comm()->tag_creator->create(
            src, comm->get_comm_id(), sched->sched_id, sched->get_op_id());
        LOG_DEBUG("PROBE entry src ", src, ", tag ", atl_tag);
        status = ccl_sched_entry_status_started;
    }

    void update() override {
        int found = 0;
        size_t len = 0;

        atl_status_t atl_status =
            comm->get_atl_comm()->probe(sched->bin->get_atl_ep(), src, atl_tag, &found, &len);

        update_status(atl_status);

        if (status == ccl_sched_entry_status_started && found) {
            LOG_DEBUG("PROBE entry done src ", src, ", tag ", atl_tag, " recv_len ", len);
            if (recv_len)
                *recv_len = len;
            status = ccl_sched_entry_status_complete;
        }
    }

    const char* name() const override {
        return class_name();
    }

protected:
    void dump_detail(std::stringstream& str) const override {
        ccl_logger::format(str,
                           "recv_len ",
                           ((recv_len) ? *recv_len : 0),
                           ", src ",
                           src,
                           ", comm_id ",
                           comm->get_comm_id(),
                           ", atl_tag ",
                           atl_tag,
                           "\n");
    }

private:
    int src;
    size_t* recv_len;
    ccl_comm* comm;
    uint64_t atl_tag = 0;
};
