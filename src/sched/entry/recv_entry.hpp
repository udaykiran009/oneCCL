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
            LOG_DEBUG("cancel RECV entry src ", src, ", req ", req, ", bytes ", bytes);
            comm->get_atl_comm()->cancel(sched->bin->get_atl_ep(), req);
        }
    }

    void start() override {
        update_fields();

        atl_tag = comm->get_atl_comm()->tag->create(
            src, comm->get_comm_id(), sched->sched_id, sched->get_op_id());
        size_t bytes = cnt * dtype.size();

        LOG_DEBUG("RECV entry src ", src, ", tag ", atl_tag, ", req ", req, ", bytes ", bytes);

        atl_status_t atl_status = comm->get_atl_comm()->recv(
            sched->bin->get_atl_ep(), buf.get_ptr(bytes), bytes, src, atl_tag, req);

        update_status(atl_status);
    }

    void update() override {
        atl_status_t atl_status = comm->get_atl_comm()->check(sched->bin->get_atl_ep(), req);

        if (unlikely(atl_status != ATL_STATUS_SUCCESS)) {
            CCL_THROW("RECV entry failed. atl_status: ", atl_status_to_str(atl_status));
        }

        if (req.is_completed) {
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
                           comm->get_comm_id(),
                           ", req ",
                           req,
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
