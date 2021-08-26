#pragma once

#include "common/global/global.hpp"
#include "sched/entry/entry.hpp"
#include "sched/queue/queue.hpp"
#include "sched/entry/copy/copy_entry.hpp"

class send_entry : public sched_entry,
                   public postponed_fields<send_entry,
                                           ccl_sched_entry_field_buf,
                                           ccl_sched_entry_field_cnt> {
public:
    static constexpr const char* class_name() noexcept {
        return "SEND";
    }

    send_entry() = delete;
    send_entry(ccl_sched* sched,
               const ccl_buffer buf,
               size_t cnt,
               const ccl_datatype& dtype,
               int dst,
               ccl_comm* comm)
            : sched_entry(sched),
              buf(buf),
              cnt(cnt),
              dtype(dtype),
              dst(dst),
              comm(comm) {}

    void start_send() {
        int global_dst = comm->get_global_rank(dst);
        int global_rank = comm->get_global_rank(comm->rank());

        atl_tag = comm->atl->tag->create(
            sched->get_comm_id(), global_rank, sched->sched_id, sched->get_op_id());
        size_t bytes = cnt * dtype.size();

        LOG_DEBUG(
            "SEND entry dst ", global_dst, ", tag ", atl_tag, ", req ", &req, ", bytes ", bytes);

        atl_status_t atl_status = comm->atl->atl_ep_send(
            sched->bin->get_atl_ep(), proxy_buf.get_ptr(bytes), bytes, global_dst, atl_tag, &req);

        update_status(atl_status);
    }

    void reset(size_t idx) override {
        sched_entry::reset(idx);
        if (proxy_copy_entry) {
            proxy_copy_entry->reset(idx);
        }
    }

    void start() override {
        update_fields();

        if (ccl::global_data::env().enable_device_buf_wa && cnt) {
            if (!proxy_buf) {
                proxy_buf = sched->alloc_buffer(cnt * dtype.size()
#ifdef CCL_ENABLE_SYCL
                                                    ,
                                                ccl_sched_buf_runtime
#endif // CCL_ENABLE_SYCL
                );
            }
            if (!proxy_copy_entry) {
                proxy_copy_entry =
                    std::shared_ptr<copy_entry>(new copy_entry(sched, buf, proxy_buf, cnt, dtype));
            }

            proxy_copy_entry->do_progress();

            if (proxy_copy_entry->get_status() != ccl_sched_entry_status_complete) {
                status = ccl_sched_entry_status_again;
                return;
            }
        }

        if (!proxy_buf) {
            proxy_buf = buf;
        }

        start_send();
    }

    void update() override {
        int req_status;

        atl_status_t atl_status =
            comm->atl->atl_ep_check(sched->bin->get_atl_ep(), &req_status, &req);

        if (unlikely(atl_status != ATL_STATUS_SUCCESS)) {
            CCL_THROW("SEND entry failed. atl_status: ", atl_status_to_str(atl_status));
        }

        if (req_status) {
            LOG_DEBUG("SEND entry done, dst ", dst);
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
                           ", dst ",
                           dst,
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
    int dst;
    ccl_comm* comm;
    uint64_t atl_tag = 0;
    atl_req_t req{};

    std::shared_ptr<copy_entry> proxy_copy_entry;
    ccl_buffer proxy_buf{};
};
