#pragma once

#include "common/global/global.hpp"
#include "sched/entry/entry.hpp"
#include "sched/queue/queue.hpp"

class recv_entry : public sched_entry,
                   public postponed_fields<recv_entry,
                                           ccl_sched_entry_field_buf,
                                           ccl_sched_entry_field_cnt> {
public:
    static constexpr const char* class_name() noexcept {
        return "RECV";
    }

    recv_entry() = delete;
    recv_entry(ccl_sched* sched,
               ccl_buffer buf,
               size_t cnt,
               const ccl_datatype& dtype,
               int src,
               ccl_comm* comm)
            : sched_entry(sched),
              buf(buf),
              cnt(cnt),
              dtype(dtype),
              src(src),
              comm(comm) {}

    ~recv_entry() {
        if (status == ccl_sched_entry_status_started) {
            size_t bytes = cnt * dtype.size();
            LOG_DEBUG("cancel RECV entry src ", src, ", req ", &req, ", bytes ", bytes);
            comm->atl->atl_ep_cancel(sched->bin->get_atl_ep(), &req);
        }
    }

    void start() override {
        update_fields();

        int global_src = comm->get_global_rank(src);
        atl_tag = comm->atl->tag->create(
            sched->get_comm_id(), global_src, sched->sched_id, sched->get_op_id());
        size_t bytes = cnt * dtype.size();

        LOG_DEBUG(
            "RECV entry src ", global_src, ", tag ", atl_tag, ", req ", &req, ", bytes ", bytes);

        atl_status_t atl_status = comm->atl->atl_ep_recv(
            sched->bin->get_atl_ep(), buf.get_ptr(bytes), bytes, global_src, atl_tag, &req);

        update_status(atl_status);
    }

    void update() override {
        int req_status;
        atl_status_t atl_status =
            comm->atl->atl_ep_check(sched->bin->get_atl_ep(), &req_status, &req);

        if (unlikely(atl_status != ATL_STATUS_SUCCESS)) {
            CCL_THROW("RECV entry failed. atl_status: ", atl_status_to_str(atl_status));
        }

        if (req_status) {
            LOG_DEBUG("RECV entry done, src ", src);
            status = ccl_sched_entry_status_complete;
        }
    }

    const char* name() const override {
        return class_name();
    }

    ccl_buffer& get_field_ref(field_id_t<ccl_sched_entry_field_buf> id) {
        return buf;
    }

    size_t& get_field_ref(field_id_t<ccl_sched_entry_field_cnt> id) {
        return cnt;
    }

protected:
    void dump_detail(std::stringstream& str) const override {
        ccl_logger::format(str,
                           "dt ",
                           ccl::global_data::get().dtypes->name(dtype),
                           ", cnt ",
                           cnt,
                           ", buf ",
                           buf,
                           ", src ",
                           src,
                           ", atl_tag ",
                           atl_tag,
                           ", comm_id ",
                           sched->get_comm_id(),
                           ", req ",
                           &req,
                           "\n");
    }

private:
    ccl_buffer buf;
    size_t cnt;
    ccl_datatype dtype;
    int src;
    ccl_comm* comm;
    uint64_t atl_tag = 0;
    atl_req_t req{};
};
